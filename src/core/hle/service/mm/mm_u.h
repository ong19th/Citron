// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2025 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/common_types.h"

namespace Core {
class System;
}

namespace Service::MM {

enum class Module : u32 {
    Cpu = 0,
    Gpu = 1,
    Emc = 2,
    SysBus = 3,
    Mselect = 4,
    Nvdec = 5,
    Nvenc = 6,
    Nvjpg = 7,
    Test = 8,
};

using Priority = u32;
using Setting = u32;

enum class EventClearMode : u32 {
    // TODO: Add specific clear mode values when documented
};

// Consolidate settings into a struct for better organization
struct Settings {
    Setting min{0};
    Setting max{0};
    Setting current{0};
    u32 id{1};  // Used by newer API versions
};

void LoopProcess(Core::System& system);

} // namespace Service::MM
