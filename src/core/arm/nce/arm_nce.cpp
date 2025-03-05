// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2025 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <cinttypes>
#include <memory>

#include "common/signal_chain.h"
#include "core/arm/nce/arm_nce.h"
#include "core/arm/nce/interpreter_visitor.h"
#include "core/arm/nce/patcher.h"
#include "core/core.h"
#include "core/memory.h"

#include "core/hle/kernel/k_process.h"

#include <signal.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace Core {

namespace {

struct sigaction g_orig_bus_action;
struct sigaction g_orig_segv_action;

// Verify assembly offsets.
using NativeExecutionParameters = Kernel::KThread::NativeExecutionParameters;
static_assert(offsetof(NativeExecutionParameters, native_context) == TpidrEl0NativeContext);
static_assert(offsetof(NativeExecutionParameters, lock) == TpidrEl0Lock);
static_assert(offsetof(NativeExecutionParameters, magic) == TpidrEl0TlsMagic);

fpsimd_context* GetFloatingPointState(mcontext_t& host_ctx) {
    _aarch64_ctx* header = reinterpret_cast<_aarch64_ctx*>(&host_ctx.__reserved);
    while (header->magic != FPSIMD_MAGIC) {
        header = reinterpret_cast<_aarch64_ctx*>(reinterpret_cast<char*>(header) + header->size);
    }
    return reinterpret_cast<fpsimd_context*>(header);
}

using namespace Common::Literals;
constexpr u32 StackSize = 128_KiB;

} // namespace

void* ArmNce::RestoreGuestContext(void* raw_context) {
    // Retrieve the host context.
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;

    // Thread-local parameters will be located in x9.
    auto* tpidr = reinterpret_cast<NativeExecutionParameters*>(host_ctx.regs[9]);
    auto* guest_ctx = static_cast<GuestContext*>(tpidr->native_context);

    // Retrieve the host floating point state.
    auto* fpctx = GetFloatingPointState(host_ctx);

    // Save host callee-saved registers.
    std::memcpy(guest_ctx->host_ctx.host_saved_vregs.data(), &fpctx->vregs[8],
                sizeof(guest_ctx->host_ctx.host_saved_vregs));
    std::memcpy(guest_ctx->host_ctx.host_saved_regs.data(), &host_ctx.regs[19],
                sizeof(guest_ctx->host_ctx.host_saved_regs));

    // Save stack pointer.
    guest_ctx->host_ctx.host_sp = host_ctx.sp;

    // Restore all guest state except tpidr_el0.
    host_ctx.sp = guest_ctx->sp;
    host_ctx.pc = guest_ctx->pc;
    host_ctx.pstate = guest_ctx->pstate;
    fpctx->fpcr = guest_ctx->fpcr;
    fpctx->fpsr = guest_ctx->fpsr;
    std::memcpy(host_ctx.regs, guest_ctx->cpu_registers.data(), sizeof(host_ctx.regs));
    std::memcpy(fpctx->vregs, guest_ctx->vector_registers.data(), sizeof(fpctx->vregs));

    // Return the new thread-local storage pointer.
    return tpidr;
}

void ArmNce::SaveGuestContext(GuestContext* guest_ctx, void* raw_context) {
    // Retrieve the host context.
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;

    // Retrieve the host floating point state.
    auto* fpctx = GetFloatingPointState(host_ctx);

    // Save all guest registers except tpidr_el0.
    std::memcpy(guest_ctx->cpu_registers.data(), host_ctx.regs, sizeof(host_ctx.regs));
    std::memcpy(guest_ctx->vector_registers.data(), fpctx->vregs, sizeof(fpctx->vregs));
    guest_ctx->fpsr = fpctx->fpsr;
    guest_ctx->fpcr = fpctx->fpcr;
    guest_ctx->pstate = static_cast<u32>(host_ctx.pstate);
    guest_ctx->pc = host_ctx.pc;
    guest_ctx->sp = host_ctx.sp;

    // Restore stack pointer.
    host_ctx.sp = guest_ctx->host_ctx.host_sp;

    // Restore host callee-saved registers.
    std::memcpy(&host_ctx.regs[19], guest_ctx->host_ctx.host_saved_regs.data(),
                sizeof(guest_ctx->host_ctx.host_saved_regs));
    std::memcpy(&fpctx->vregs[8], guest_ctx->host_ctx.host_saved_vregs.data(),
                sizeof(guest_ctx->host_ctx.host_saved_vregs));

    // Return from the call on exit by setting pc to x30.
    host_ctx.pc = guest_ctx->host_ctx.host_saved_regs[11];

    // Clear esr_el1 and return it.
    host_ctx.regs[0] = guest_ctx->esr_el1.exchange(0);
}

bool ArmNce::HandleFailedGuestFault(GuestContext* guest_ctx, void* raw_info, void* raw_context) {
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;
    auto* info = static_cast<siginfo_t*>(raw_info);

    // We can't handle the access, so determine why we crashed.
    const bool is_prefetch_abort = host_ctx.pc == reinterpret_cast<u64>(info->si_addr);

    // For data aborts, skip the instruction and return to guest code.
    // This will allow games to continue in many scenarios where they would otherwise crash.
    if (!is_prefetch_abort) {
        host_ctx.pc += 4;
        return true;
    }

    // This is a prefetch abort.
    guest_ctx->esr_el1.fetch_or(static_cast<u64>(HaltReason::PrefetchAbort));

    // Forcibly mark the context as locked. We are still running.
    // We may race with SignalInterrupt here:
    // - If we lose the race, then SignalInterrupt will send us a signal we are masking,
    //   and it will do nothing when it is unmasked, as we have already left guest code.
    // - If we win the race, then SignalInterrupt will wait for us to unlock first.
    auto& thread_params = guest_ctx->parent->m_running_thread->GetNativeExecutionParameters();
    thread_params.lock.store(SpinLockLocked);

    // Return to host.
    SaveGuestContext(guest_ctx, raw_context);
    return false;
}

bool ArmNce::HandleGuestAlignmentFault(GuestContext* guest_ctx, void* raw_info, void* raw_context) {
    auto& host_ctx = static_cast<ucontext_t*>(raw_context)->uc_mcontext;
    auto* fpctx = GetFloatingPointState(host_ctx);
    auto& memory = guest_ctx->system->ApplicationMemory();

    // Log the alignment fault for debugging
    LOG_DEBUG(Core_ARM, "Alignment fault at PC={:X}", host_ctx.pc);

    // Try to handle the instruction
    auto next_pc = MatchAndExecuteOneInstruction(memory, &host_ctx, fpctx);
    if (next_pc) {
        host_ctx.pc = *next_pc;
        return true;
    }

    // If we couldn't handle it, try to skip the instruction as a fallback
    LOG_DEBUG(Core_ARM, "Could not handle alignment fault, skipping instruction");
    host_ctx.pc += 4; // Skip to next instruction

    // Return true to continue execution
    return true;
}

bool ArmNce::HandleGuestAccessFault(GuestContext* guest_ctx, void* raw_info, void* raw_context) {
    auto* info = static_cast<siginfo_t*>(raw_info);
    const u64 fault_addr = reinterpret_cast<u64>(info->si_addr);
    auto& memory = guest_ctx->system->ApplicationMemory();

    // Get the ArmNce instance from the guest context
    ArmNce* nce = guest_ctx->parent;

    // Define a maximum retry count to prevent infinite loops
    constexpr int max_retries = 3;
    int retry_count = 0;

    while (retry_count < max_retries) {
        // Check TLB first
        if (TlbEntry* entry = nce->FindTlbEntry(fault_addr)) {
            if (!entry->writable && info->si_code == SEGV_ACCERR) {
                LOG_DEBUG(Core_ARM, "Write to read-only memory at {:X}", fault_addr);
                return HandleFailedGuestFault(guest_ctx, raw_info, raw_context);
            }
            return true;
        }

        // TLB miss handling with better error checking
        if (memory.InvalidateNCE(fault_addr, Memory::CITRON_PAGESIZE)) {
            const u64 host_addr = reinterpret_cast<u64>(memory.GetPointer(fault_addr));

            if (host_addr) {
                nce->AddTlbEntry(fault_addr, host_addr, Memory::CITRON_PAGESIZE, true);
                return true;
            } else {
                LOG_DEBUG(Core_ARM, "Failed to get host address for guest address {:X}", fault_addr);
            }
        } else {
            LOG_DEBUG(Core_ARM, "Memory invalidation failed for address {:X}", fault_addr);
        }

        // Trigger an immediate remap if lookup fails
        if (!memory.Remap(fault_addr, Memory::CITRON_PAGESIZE)) {
            LOG_ERROR(Core_ARM, "Immediate remap failed for address {:X}", fault_addr);
            return HandleFailedGuestFault(guest_ctx, raw_info, raw_context);
        }

        // Retry adding the TLB entry after remap
        const u64 host_addr = reinterpret_cast<u64>(memory.GetPointer(fault_addr));
        if (host_addr) {
            nce->AddTlbEntry(fault_addr, host_addr, Memory::CITRON_PAGESIZE, true);
            return true;
        } else {
            LOG_ERROR(Core_ARM, "Failed to get host address after remap for guest address {:X}", fault_addr);
        }

        // Increment the retry count
        retry_count++;
    }

    // If all retries fail, handle the fault as a failed guest fault
    return HandleFailedGuestFault(guest_ctx, raw_info, raw_context);
}

void ArmNce::HandleHostAlignmentFault(int sig, void* raw_info, void* raw_context) {
    return g_orig_bus_action.sa_sigaction(sig, static_cast<siginfo_t*>(raw_info), raw_context);
}

void ArmNce::HandleHostAccessFault(int sig, void* raw_info, void* raw_context) {
    return g_orig_segv_action.sa_sigaction(sig, static_cast<siginfo_t*>(raw_info), raw_context);
}

void ArmNce::LockThread(Kernel::KThread* thread) {
    auto* thread_params = &thread->GetNativeExecutionParameters();
    LockThreadParameters(thread_params);
}

void ArmNce::UnlockThread(Kernel::KThread* thread) {
    auto* thread_params = &thread->GetNativeExecutionParameters();
    UnlockThreadParameters(thread_params);
}

HaltReason ArmNce::RunThread(Kernel::KThread* thread) {
    // Check if we're already interrupted.
    // If we are, we can just return immediately.
    HaltReason hr = static_cast<HaltReason>(m_guest_ctx.esr_el1.exchange(0));
    if (True(hr)) {
        return hr;
    }

    // Get the thread context.
    auto* thread_params = &thread->GetNativeExecutionParameters();
    auto* process = thread->GetOwnerProcess();

    // Assign current members.
    m_running_thread = thread;
    m_guest_ctx.parent = this;
    thread_params->native_context = &m_guest_ctx;
    thread_params->tpidr_el0 = m_guest_ctx.tpidr_el0;
    thread_params->tpidrro_el0 = m_guest_ctx.tpidrro_el0;
    thread_params->is_running = true;

    // TODO: finding and creating the post handler needs to be locked
    // to deal with dynamic loading of NROs.
    const auto& post_handlers = process->GetPostHandlers();
    if (auto it = post_handlers.find(m_guest_ctx.pc); it != post_handlers.end()) {
        hr = ReturnToRunCodeByTrampoline(thread_params, &m_guest_ctx, it->second);
    } else {
        hr = ReturnToRunCodeByExceptionLevelChange(m_thread_id, thread_params);
    }

    // Unload members.
    // The thread does not change, so we can persist the old reference.
    m_running_thread = nullptr;
    m_guest_ctx.tpidr_el0 = thread_params->tpidr_el0;
    thread_params->native_context = nullptr;
    thread_params->is_running = false;

    // Return the halt reason.
    return hr;
}

HaltReason ArmNce::StepThread(Kernel::KThread* thread) {
    return HaltReason::StepThread;
}

u32 ArmNce::GetSvcNumber() const {
    return m_guest_ctx.svc;
}

void ArmNce::GetSvcArguments(std::span<uint64_t, 8> args) const {
    for (size_t i = 0; i < 8; i++) {
        args[i] = m_guest_ctx.cpu_registers[i];
    }
}

void ArmNce::SetSvcArguments(std::span<const uint64_t, 8> args) {
    for (size_t i = 0; i < 8; i++) {
        m_guest_ctx.cpu_registers[i] = args[i];
    }
}

ArmNce::ArmNce(System& system, bool uses_wall_clock, std::size_t core_index)
    : ArmInterface{uses_wall_clock}, m_system{system}, m_core_index{core_index} {
    m_guest_ctx.system = &m_system;
    StartTlbInvalidationTimer(); // Add this line
}

ArmNce::~ArmNce() = default;

void ArmNce::Initialize() {
    if (m_thread_id == -1) {
        m_thread_id = gettid();
    }

    // Configure signal stack.
    if (!m_stack) {
        m_stack = std::make_unique<u8[]>(StackSize);

        stack_t ss{};
        ss.ss_sp = m_stack.get();
        ss.ss_size = StackSize;
        sigaltstack(&ss, nullptr);
    }

    // Set up signals.
    static std::once_flag flag;
    std::call_once(flag, [] {
        using HandlerType = decltype(sigaction::sa_sigaction);

        sigset_t signal_mask;
        sigemptyset(&signal_mask);
        sigaddset(&signal_mask, ReturnToRunCodeByExceptionLevelChangeSignal);
        sigaddset(&signal_mask, BreakFromRunCodeSignal);
        sigaddset(&signal_mask, GuestAlignmentFaultSignal);
        sigaddset(&signal_mask, GuestAccessFaultSignal);

        struct sigaction return_to_run_code_action {};
        return_to_run_code_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        return_to_run_code_action.sa_sigaction = reinterpret_cast<HandlerType>(
            &ArmNce::ReturnToRunCodeByExceptionLevelChangeSignalHandler);
        return_to_run_code_action.sa_mask = signal_mask;
        Common::SigAction(ReturnToRunCodeByExceptionLevelChangeSignal, &return_to_run_code_action,
                          nullptr);

        struct sigaction break_from_run_code_action {};
        break_from_run_code_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        break_from_run_code_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::BreakFromRunCodeSignalHandler);
        break_from_run_code_action.sa_mask = signal_mask;
        Common::SigAction(BreakFromRunCodeSignal, &break_from_run_code_action, nullptr);

        struct sigaction alignment_fault_action {};
        alignment_fault_action.sa_flags = SA_SIGINFO | SA_ONSTACK;
        alignment_fault_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::GuestAlignmentFaultSignalHandler);
        alignment_fault_action.sa_mask = signal_mask;
        Common::SigAction(GuestAlignmentFaultSignal, &alignment_fault_action, nullptr);

        struct sigaction access_fault_action {};
        access_fault_action.sa_flags = SA_SIGINFO | SA_ONSTACK | SA_RESTART;
        access_fault_action.sa_sigaction =
            reinterpret_cast<HandlerType>(&ArmNce::GuestAccessFaultSignalHandler);
        access_fault_action.sa_mask = signal_mask;
        Common::SigAction(GuestAccessFaultSignal, &access_fault_action, &g_orig_segv_action);
    });
}

void ArmNce::SetTpidrroEl0(u64 value) {
    m_guest_ctx.tpidrro_el0 = value;
}

void ArmNce::GetContext(Kernel::Svc::ThreadContext& ctx) const {
    for (size_t i = 0; i < 29; i++) {
        ctx.r[i] = m_guest_ctx.cpu_registers[i];
    }
    ctx.fp = m_guest_ctx.cpu_registers[29];
    ctx.lr = m_guest_ctx.cpu_registers[30];
    ctx.sp = m_guest_ctx.sp;
    ctx.pc = m_guest_ctx.pc;
    ctx.pstate = m_guest_ctx.pstate;
    ctx.v = m_guest_ctx.vector_registers;
    ctx.fpcr = m_guest_ctx.fpcr;
    ctx.fpsr = m_guest_ctx.fpsr;
    ctx.tpidr = m_guest_ctx.tpidr_el0;
}

void ArmNce::SetContext(const Kernel::Svc::ThreadContext& ctx) {
    for (size_t i = 0; i < 29; i++) {
        m_guest_ctx.cpu_registers[i] = ctx.r[i];
    }
    m_guest_ctx.cpu_registers[29] = ctx.fp;
    m_guest_ctx.cpu_registers[30] = ctx.lr;
    m_guest_ctx.sp = ctx.sp;
    m_guest_ctx.pc = ctx.pc;
    m_guest_ctx.pstate = ctx.pstate;
    m_guest_ctx.vector_registers = ctx.v;
    m_guest_ctx.fpcr = ctx.fpcr;
    m_guest_ctx.fpsr = ctx.fpsr;
    m_guest_ctx.tpidr_el0 = ctx.tpidr;
}

void ArmNce::SignalInterrupt(Kernel::KThread* thread) {
    // Add break loop condition.
    m_guest_ctx.esr_el1.fetch_or(static_cast<u64>(HaltReason::BreakLoop));

    // Lock the thread context.
    auto* params = &thread->GetNativeExecutionParameters();
    LockThreadParameters(params);

    if (params->is_running) {
        // We should signal to the running thread.
        // The running thread will unlock the thread context.
        syscall(SYS_tkill, m_thread_id, BreakFromRunCodeSignal);
    } else {
        // If the thread is no longer running, we have nothing to do.
        UnlockThreadParameters(params);
    }
}

void ArmNce::ClearInstructionCache() {
    // TODO: This is not possible to implement correctly on Linux because
    // we do not have any access to ic iallu.

    // Require accesses to complete.
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

void ArmNce::InvalidateCacheRange(u64 addr, std::size_t size) {
    this->ClearInstructionCache();
}

TlbEntry* ArmNce::FindTlbEntry(u64 guest_addr) {
    std::lock_guard lock(m_tlb_mutex);

    // Simple linear search - more reliable than complex indexing
    for (size_t i = 0; i < TLB_SIZE; i++) {
        TlbEntry& entry = m_tlb[i];
        if (entry.valid &&
            guest_addr >= entry.guest_addr &&
            guest_addr < (entry.guest_addr + entry.size)) {

            // Simple access tracking - just increment counter
            if (entry.access_count < 1000) { // Prevent overflow
                entry.access_count++;
            }
            entry.ref_count++; // Increment reference count
            return &entry;
        }
    }
    return nullptr;
}

void ArmNce::AddTlbEntry(u64 guest_addr, u64 host_addr, u32 size, bool writable) {
    if (!host_addr) {
        LOG_ERROR(Core_ARM, "Invalid host address for guest address {:X}", guest_addr);
        return;
    }

    std::lock_guard lock(m_tlb_mutex);

    size_t replace_idx = TLB_SIZE;
    auto now = std::chrono::steady_clock::now();

    // First try to find an invalid entry
    for (size_t i = 0; i < TLB_SIZE; i++) {
        if (!m_tlb[i].valid) {
            replace_idx = i;
            break;
        }
    }

    // If no invalid entries, use LRU or access count policy
    if (replace_idx == TLB_SIZE) {
        u32 lowest_count = UINT32_MAX;
        auto oldest_time = now;
        for (size_t i = 0; i < TLB_SIZE; i++) {
            if (m_tlb[i].access_count < lowest_count) {
                lowest_count = m_tlb[i].access_count;
                replace_idx = i;
            }
            if (m_tlb[i].last_access_time < oldest_time) {
                oldest_time = m_tlb[i].last_access_time;
                replace_idx = i;
            }
        }
    }

    if (replace_idx >= TLB_SIZE) {
        replace_idx = 0;
    }

    const u64 page_mask = size - 1;
    const u64 aligned_guest = guest_addr & ~page_mask;
    const u64 aligned_host = host_addr & ~page_mask;

    m_tlb[replace_idx] = {
        .guest_addr = aligned_guest,
        .host_addr = aligned_host,
        .size = size,
        .valid = true,
        .writable = writable,
        .access_count = 1,
        .last_access_time = now,
        .ref_count = 1 // Initialize reference count
    };
}

void ArmNce::InvalidateTlb() {
    std::lock_guard lock(m_tlb_mutex);
    auto now = std::chrono::steady_clock::now();
    auto expiration_time = std::chrono::minutes(5); // Example: 5 minutes expiration

    for (auto& entry : m_tlb) {
        if (entry.valid) {
            if (entry.access_count > 1000 || (now - entry.last_access_time) > expiration_time) {
                entry.valid = false;
            }
        }
    }
}

void ArmNce::StartTlbInvalidationTimer() {
    std::thread([this]() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::minutes(1)); // Example: check every minute
            this->InvalidateTlb();
        }
    }).detach();
}

struct TlbEntry {
    u64 guest_addr;
    u64 host_addr;
    u32 size;
    bool valid;
    bool writable;
    u32 access_count;
    std::chrono::steady_clock::time_point last_access_time; // Add this line
    u32 ref_count; // Add this line
};

} // namespace Core
