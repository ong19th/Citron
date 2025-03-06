// SPDX-FileCopyrightText: Copyright 2023 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2025 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mutex>
#include <array>

#include "core/arm/arm_interface.h"
#include "core/arm/nce/guest_context.h"

namespace Core::Memory {
class Memory;
}

namespace Core {

class System;

struct TlbEntry {
    u64 guest_addr;
    u64 host_addr;
    u32 size;
    bool valid;
    bool writable;
    u64 last_access_time; // For LRU tracking
    u32 access_count;     // For access frequency tracking
};

// Improved TLB configuration
constexpr size_t TLB_SETS = 64;     // Number of sets
constexpr size_t TLB_WAYS = 8;      // Ways per set
constexpr size_t TLB_SIZE = TLB_SETS * TLB_WAYS;

class ArmNce final : public ArmInterface {
public:
    ArmNce(System& system, bool uses_wall_clock, std::size_t core_index);
    ~ArmNce() override;

    void Initialize() override;

    Architecture GetArchitecture() const override {
        return Architecture::AArch64;
    }

    HaltReason RunThread(Kernel::KThread* thread) override;
    HaltReason StepThread(Kernel::KThread* thread) override;

    void GetContext(Kernel::Svc::ThreadContext& ctx) const override;
    void SetContext(const Kernel::Svc::ThreadContext& ctx) override;
    void SetTpidrroEl0(u64 value) override;

    void GetSvcArguments(std::span<uint64_t, 8> args) const override;
    void SetSvcArguments(std::span<const uint64_t, 8> args) override;
    u32 GetSvcNumber() const override;

    void SignalInterrupt(Kernel::KThread* thread) override;
    void ClearInstructionCache() override;
    void InvalidateCacheRange(u64 addr, std::size_t size) override;

    void LockThread(Kernel::KThread* thread) override;
    void UnlockThread(Kernel::KThread* thread) override;

protected:
    const Kernel::DebugWatchpoint* HaltedWatchpoint() const override {
        return nullptr;
    }

    void RewindBreakpointInstruction() override {}

private:
    // Assembly definitions.
    static HaltReason ReturnToRunCodeByTrampoline(void* tpidr, GuestContext* ctx,
                                                  u64 trampoline_addr);
    static HaltReason ReturnToRunCodeByExceptionLevelChange(int tid, void* tpidr);

    static void ReturnToRunCodeByExceptionLevelChangeSignalHandler(int sig, void* info,
                                                                   void* raw_context);
    static void BreakFromRunCodeSignalHandler(int sig, void* info, void* raw_context);
    static void GuestAlignmentFaultSignalHandler(int sig, void* info, void* raw_context);
    static void GuestAccessFaultSignalHandler(int sig, void* info, void* raw_context);

    static void LockThreadParameters(void* tpidr);
    static void UnlockThreadParameters(void* tpidr);

private:
    // C++ implementation functions for assembly definitions.
    static void* RestoreGuestContext(void* raw_context);
    static void SaveGuestContext(GuestContext* ctx, void* raw_context);
    static bool HandleFailedGuestFault(GuestContext* ctx, void* info, void* raw_context);
    static bool HandleGuestAlignmentFault(GuestContext* ctx, void* info, void* raw_context);
    static bool HandleGuestAccessFault(GuestContext* ctx, void* info, void* raw_context);
    static void HandleHostAlignmentFault(int sig, void* info, void* raw_context);
    static void HandleHostAccessFault(int sig, void* info, void* raw_context);

public:
    Core::System& m_system;

    // Members set on initialization.
    std::size_t m_core_index{};
    pid_t m_thread_id{-1};

    // Core context.
    GuestContext m_guest_ctx{};
    Kernel::KThread* m_running_thread{};

    // Stack for signal processing.
    std::unique_ptr<u8[]> m_stack{};

    // Enhanced TLB implementation
    std::array<TlbEntry, TLB_SIZE> m_tlb{};
    std::mutex m_tlb_mutex;
    u64 m_tlb_access_counter{0};

    // TLB helper functions
    TlbEntry* FindTlbEntry(u64 guest_addr);
    void AddTlbEntry(u64 guest_addr, u64 host_addr, u32 size, bool writable);
    void InvalidateTlb();
    size_t GetTlbSetIndex(u64 guest_addr) const;
    size_t FindReplacementEntry(size_t set_start);
    void UpdateTlbEntryStats(TlbEntry& entry);

    // Thread context caching
    std::mutex m_context_mutex;
    Kernel::KThread* m_last_thread{nullptr};
    GuestContext m_cached_ctx{};
};

} // namespace Core
