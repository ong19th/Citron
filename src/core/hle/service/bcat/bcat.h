// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project & 2025 citron Homebrew Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/service.h"

namespace Core {
class System;
}

namespace Service::BCAT {

void LoopProcess(Core::System& system);

} // namespace Service::BCAT
