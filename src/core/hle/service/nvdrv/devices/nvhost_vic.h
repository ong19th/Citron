// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project & 2025 citron Homebrew Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/nvdrv/devices/nvhost_nvdec_common.h"

namespace Service::Nvidia::Devices {

class nvhost_vic final : public nvhost_nvdec_common {
public:
    explicit nvhost_vic(Core::System& system_, NvCore::Container& core);
    ~nvhost_vic();

    NvResult Ioctl1(DeviceFD fd, Ioctl command, std::span<const u8> input,
                    std::span<u8> output) override;
    NvResult Ioctl2(DeviceFD fd, Ioctl command, std::span<const u8> input,
                    std::span<const u8> inline_input, std::span<u8> output) override;
    NvResult Ioctl3(DeviceFD fd, Ioctl command, std::span<const u8> input, std::span<u8> output,
                    std::span<u8> inline_output) override;

    void OnOpen(NvCore::SessionId session_id, DeviceFD fd) override;
    void OnClose(DeviceFD fd) override;
};
} // namespace Service::Nvidia::Devices
