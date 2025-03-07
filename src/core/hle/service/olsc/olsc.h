// SPDX-FileCopyrightText: Copyright 2020 yuzu Emulator Project & 2025 citron Homebrew Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

namespace Core {
class System;
}

namespace Service::OLSC {

void LoopProcess(Core::System& system);

} // namespace Service::OLSC
