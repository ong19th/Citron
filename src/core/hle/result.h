// SPDX-FileCopyrightText: 2014 Citra Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/bit_field.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/expected.h"

// All the constants in this file come from http://switchbrew.org/index.php?title=Error_codes

/**
 * Identifies the module which caused the error. Error codes can be propagated through a call
 * chain, meaning that this doesn't always correspond to the module where the API call made is
 * contained.
 */
enum class ErrorModule : u32 {
    // 0-9
    Common = 0,
    Svc = 1,
    Fs = 2,
    Os = 3,
    Htcs = 4,
    Ncm = 5,
    Dd = 6,
    Osdbg = 7,
    Lr = 8,
    Ldr = 9,

    // 10-19
    Sf = 10,
    SfHipc = 11,
    Tma = 12,
    Dmnt = 13,
    Gds = 14,
    Pm = 15,
    Ns = 16,
    BsdSockets = 17,
    Htc = 18,
    Tsc = 19,

    // 20-29
    Kvdb = 20,
    Sm = 21,
    Ro = 22,
    Gc = 23,
    Sdmmc = 24,
    Ovln = 25,
    Spl = 26,
    Socket = 27,
    Htclow = 29,

    // 30-39
    Ddsf = 30,
    Htcfs = 31,
    Async = 32,
    Util = 33,
    Tipc = 35,
    Anif = 37,
    Crt = 39,

    // 100-109
    Eth = 100,
    I2c = 101,
    Gpio = 102,
    Uart = 103,
    Cpad = 104,
    Settings = 105,
    Ftm = 106,
    Wlan = 107,
    Xcd = 108,
    Tmp451 = 109,

    // 110-119
    Nifm = 110,
    Codec = 111,
    Lsm6ds3 = 112,
    Bluetooth = 113,
    Vi = 114,
    Nfp = 115,
    Time = 116,
    Fgm = 117,
    Oe = 118,
    Bh1730fvc = 119,

    // 120-129
    Pcie = 120,
    Friends = 121,
    Bcat = 122,
    Ssl = 123,
    Account = 124,
    News = 125,
    Mii = 126,
    Nfc = 127,
    Am = 128,
    Prepo = 129,

    // 130-139
    Ahid = 130,
    Applet = 131,
    Ae = 132,
    Pcv = 133,
    UsbPd = 134,
    Bpc = 135,
    Psm = 136,
    Nim = 137,
    Psc = 138,
    Tc = 139,

    // 140-149
    Usb = 140,
    Nsd = 141,
    Pctl = 142,
    Btm = 143,
    La = 144,
    Es = 145,
    Ngc = 146,
    Erpt = 147,
    Apm = 148,
    Cec = 149,

    // 150-159
    Profiler = 150,
    Eupld = 151,
    Lidbe = 152,
    Audio = 153,
    Npns = 154,
    Http = 155,
    Idle = 156,
    Arp = 157,
    Updater = 158,
    Swkbd = 159,

    // 160-169
    Netdiag = 160,
    NfcMifare = 161,
    Err = 162,
    Fatal = 163,
    Ec = 164,
    Spsm = 165,
    Aoc = 166,
    Bgtc = 167,
    Creport = 168,
    Sasbus = 169,

    // 170-179
    Pl = 170,
    Audioctrl = 172,
    Lbl = 173,
    Jit = 175,
    Hdcp = 176,
    Omm = 177,
    Pdm = 178,
    Olsc = 179,

    // 180-189
    Srepo = 180,
    Dauth = 181,
    Stdfu = 182,
    Dbg = 183,
    Dhcps = 186,
    Spi = 187,
    Avm = 188,
    Pwm = 189,

    // 190-199
    Rtc = 191,
    Regulator = 192,
    Led = 193,
    Sio = 195,
    Pcm = 196,
    Clkrst = 197,
    Powctl = 198,

    // 200-209
    AudioOld = 201,
    Hid = 202,
    Ldn = 203,
    Cs = 204,
    Irsensor = 205,
    Capsrv = 206,
    Manu = 208,
    Atk = 209,

    // 210-219
    Web = 210,
    Lcs = 211,
    Grc = 212,
    Repair = 213,
    Album = 214,
    Rid = 215,
    Migration = 216,
    MigrationIdc = 217,
    Hidbus = 218,
    Ens = 219,

    // 220-229
    Websocket = 223,
    Dcdmtp = 227,
    Pgl = 228,
    Notification = 229,

    // 230-239
    Ins = 230,
    Lp2p = 231,
    Rcd = 232,
    Icm40607 = 233,
    Prc = 235,
    TmaHtc = 237,
    Ectx = 238,
    Mnpp = 239,

    // 240-249
    Hshl = 240,
    Capmtp = 242,
    Dp2hdmi = 244,
    Cradle = 245,
    Sprofile = 246,

    // 250-299
    Ndrm = 250,
    Fst2 = 251,
    Nex = 306,

    // 300-399
    Npln = 321,
    LibNx = 345,
    HomebrewAbi = 346,
    HomebrewLoader = 347,
    LibNxNvidia = 348,
    LibNxBinder = 349,

    // 400-499
    Tspm = 499,

    // 500-599
    Devmenu = 500,
    Nverpt = 520,
    AmStuckMonitor = 521,

    // 600-699
    Pia = 618,
    Eagle = 623,

    // 800-899
    GeneralWebApplet = 800,
    WifiWebAuthApplet = 809,
    WhitelistedApplet = 810,
    ShopN = 811,
    Coral = 815,
};

/// Encapsulates a Horizon OS error code, allowing it to be separated into its constituent fields.
union Result {
    u32 raw;

    using Module = BitField<0, 9, ErrorModule>;
    using Description = BitField<9, 13, u32>;

    Result() = default;
    constexpr explicit Result(u32 raw_) : raw(raw_) {}

    constexpr Result(ErrorModule module_, u32 description_)
        : raw(Module::FormatValue(module_) | Description::FormatValue(description_)) {}

    [[nodiscard]] constexpr bool IsSuccess() const {
        return raw == 0;
    }

    [[nodiscard]] constexpr bool IsError() const {
        return !IsSuccess();
    }

    [[nodiscard]] constexpr bool IsFailure() const {
        return !IsSuccess();
    }

    [[nodiscard]] constexpr u32 GetInnerValue() const {
        return raw;
    }

    [[nodiscard]] constexpr ErrorModule GetModule() const {
        return Module::ExtractValue(raw);
    }

    [[nodiscard]] constexpr u32 GetDescription() const {
        return Description::ExtractValue(raw);
    }

    [[nodiscard]] constexpr bool Includes(Result result) const {
        return GetInnerValue() == result.GetInnerValue();
    }
};
static_assert(std::is_trivial_v<Result>);

[[nodiscard]] constexpr bool operator==(const Result& a, const Result& b) {
    return a.raw == b.raw;
}

[[nodiscard]] constexpr bool operator!=(const Result& a, const Result& b) {
    return !operator==(a, b);
}

// Convenience functions for creating some common kinds of errors:

/// The default success `Result`.
constexpr Result ResultSuccess(0);

/**
 * Placeholder result code used for unknown error codes.
 *
 * @note This should only be used when a particular error code
 *       is not known yet.
 */
constexpr Result ResultUnknown(UINT32_MAX);

/**
 * A ResultRange defines an inclusive range of error descriptions within an error module.
 * This can be used to check whether the description of a given Result falls within the range.
 * The conversion function returns a Result with its description set to description_start.
 *
 * An example of how it could be used:
 * \code
 * constexpr ResultRange ResultCommonError{ErrorModule::Common, 0, 9999};
 *
 * Result Example(int value) {
 *     const Result result = OtherExample(value);
 *
 *     // This will only evaluate to true if result.module is ErrorModule::Common and
 *     // result.description is in between 0 and 9999 inclusive.
 *     if (ResultCommonError.Includes(result)) {
 *         // This returns Result{ErrorModule::Common, 0};
 *         return ResultCommonError;
 *     }
 *
 *     return ResultSuccess;
 * }
 * \endcode
 */
class ResultRange {
public:
    consteval ResultRange(ErrorModule module, u32 description_start, u32 description_end_)
        : code{module, description_start}, description_end{description_end_} {}

    [[nodiscard]] constexpr operator Result() const {
        return code;
    }

    [[nodiscard]] constexpr bool Includes(Result other) const {
        return code.GetModule() == other.GetModule() &&
               code.GetDescription() <= other.GetDescription() &&
               other.GetDescription() <= description_end;
    }

private:
    Result code;
    u32 description_end;
};

#define R_SUCCEEDED(res) (static_cast<Result>(res).IsSuccess())
#define R_FAILED(res) (static_cast<Result>(res).IsFailure())

namespace ResultImpl {
template <auto EvaluateResult, class F>
class ScopedResultGuard {
    CITRON_NON_COPYABLE(ScopedResultGuard);
    CITRON_NON_MOVEABLE(ScopedResultGuard);

private:
    Result& m_ref;
    F m_f;

public:
    constexpr ScopedResultGuard(Result& ref, F f) : m_ref(ref), m_f(std::move(f)) {}
    constexpr ~ScopedResultGuard() {
        if (EvaluateResult(m_ref)) {
            m_f();
        }
    }
};

template <auto EvaluateResult>
class ResultReferenceForScopedResultGuard {
private:
    Result& m_ref;

public:
    constexpr ResultReferenceForScopedResultGuard(Result& r) : m_ref(r) {}
    constexpr operator Result&() const {
        return m_ref;
    }
};

template <auto EvaluateResult, typename F>
constexpr ScopedResultGuard<EvaluateResult, F> operator+(
    ResultReferenceForScopedResultGuard<EvaluateResult> ref, F&& f) {
    return ScopedResultGuard<EvaluateResult, F>(static_cast<Result&>(ref), std::forward<F>(f));
}

constexpr bool EvaluateResultSuccess(const Result& r) {
    return R_SUCCEEDED(r);
}
constexpr bool EvaluateResultFailure(const Result& r) {
    return R_FAILED(r);
}

template <auto... R>
constexpr bool EvaluateAnyResultIncludes(const Result& r) {
    return ((r == R) || ...);
}

template <auto... R>
constexpr bool EvaluateResultNotIncluded(const Result& r) {
    return !EvaluateAnyResultIncludes<R...>(r);
}

template <typename T>
constexpr void UpdateCurrentResultReference(T result_reference, Result result) = delete;
// Intentionally not defined

template <>
constexpr void UpdateCurrentResultReference<Result&>(Result& result_reference, Result result) {
    result_reference = result;
}

template <>
constexpr void UpdateCurrentResultReference<const Result>(Result result_reference, Result result) {}
} // namespace ResultImpl

#define DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(COUNTER_VALUE)                                \
    [[maybe_unused]] constexpr bool CONCAT2(HasPrevRef_, COUNTER_VALUE) =                          \
        std::same_as<decltype(__TmpCurrentResultReference), Result&>;                              \
    [[maybe_unused]] Result CONCAT2(PrevRef_, COUNTER_VALUE) = __TmpCurrentResultReference;        \
    [[maybe_unused]] Result CONCAT2(__tmp_result_, COUNTER_VALUE) = ResultSuccess;                 \
    Result& __TmpCurrentResultReference = CONCAT2(HasPrevRef_, COUNTER_VALUE)                      \
                                              ? CONCAT2(PrevRef_, COUNTER_VALUE)                   \
                                              : CONCAT2(__tmp_result_, COUNTER_VALUE)

#define ON_RESULT_RETURN_IMPL(...)                                                                 \
    static_assert(std::same_as<decltype(__TmpCurrentResultReference), Result&>);                   \
    auto CONCAT2(RESULT_GUARD_STATE_, __COUNTER__) =                                               \
        ResultImpl::ResultReferenceForScopedResultGuard<__VA_ARGS__>(                              \
            __TmpCurrentResultReference) +                                                         \
        [&]()

#define ON_RESULT_FAILURE_2 ON_RESULT_RETURN_IMPL(ResultImpl::EvaluateResultFailure)

#define ON_RESULT_FAILURE                                                                          \
    DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                     \
    ON_RESULT_FAILURE_2

#define ON_RESULT_SUCCESS_2 ON_RESULT_RETURN_IMPL(ResultImpl::EvaluateResultSuccess)

#define ON_RESULT_SUCCESS                                                                          \
    DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                     \
    ON_RESULT_SUCCESS_2

#define ON_RESULT_INCLUDED_2(...)                                                                  \
    ON_RESULT_RETURN_IMPL(ResultImpl::EvaluateAnyResultIncludes<__VA_ARGS__>)

#define ON_RESULT_INCLUDED(...)                                                                    \
    DECLARE_CURRENT_RESULT_REFERENCE_AND_STORAGE(__COUNTER__);                                     \
    ON_RESULT_INCLUDED_2(__VA_ARGS__)

constexpr inline Result __TmpCurrentResultReference = ResultSuccess;

/// Returns a result.
#define R_RETURN(res_expr)                                                                         \
    {                                                                                              \
        const Result _tmp_r_throw_rc = (res_expr);                                                 \
        ResultImpl::UpdateCurrentResultReference<decltype(__TmpCurrentResultReference)>(           \
            __TmpCurrentResultReference, _tmp_r_throw_rc);                                         \
        return _tmp_r_throw_rc;                                                                    \
    }

/// Returns ResultSuccess()
#define R_SUCCEED() R_RETURN(ResultSuccess)

/// Throws a result.
#define R_THROW(res_expr) R_RETURN(res_expr)

/// Evaluates a boolean expression, and returns a result unless that expression is true.
#define R_UNLESS(expr, res)                                                                        \
    {                                                                                              \
        if (!(expr)) {                                                                             \
            R_THROW(res);                                                                          \
        }                                                                                          \
    }

/// Evaluates an expression that returns a result, and returns the result if it would fail.
#define R_TRY(res_expr)                                                                            \
    {                                                                                              \
        const auto _tmp_r_try_rc = (res_expr);                                                     \
        if (R_FAILED(_tmp_r_try_rc)) {                                                             \
            R_THROW(_tmp_r_try_rc);                                                                \
        }                                                                                          \
    }

/// Evaluates a boolean expression, and succeeds if that expression is true.
#define R_SUCCEED_IF(expr) R_UNLESS(!(expr), ResultSuccess)

#define R_TRY_CATCH(res_expr)                                                                      \
    {                                                                                              \
        const auto R_CURRENT_RESULT = (res_expr);                                                  \
        if (R_FAILED(R_CURRENT_RESULT)) {                                                          \
            if (false)

#define R_END_TRY_CATCH                                                                            \
    else if (R_FAILED(R_CURRENT_RESULT)) {                                                         \
        R_THROW(R_CURRENT_RESULT);                                                                 \
    }                                                                                              \
    }                                                                                              \
    }

#define R_CATCH_ALL()                                                                              \
    }                                                                                              \
    else if (R_FAILED(R_CURRENT_RESULT)) {                                                         \
        if (true)

#define R_CATCH(res_expr)                                                                          \
    }                                                                                              \
    else if ((res_expr) == (R_CURRENT_RESULT)) {                                                   \
        if (true)

#define R_CONVERT(catch_type, convert_type)                                                        \
    R_CATCH(catch_type) { R_THROW(static_cast<Result>(convert_type)); }

#define R_CONVERT_ALL(convert_type)                                                                \
    R_CATCH_ALL() { R_THROW(static_cast<Result>(convert_type)); }

#define R_ASSERT(res_expr) ASSERT(R_SUCCEEDED(res_expr))
