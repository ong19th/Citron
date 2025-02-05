// SPDX-FileCopyrightText: Copyright 2024 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "core/hle/service/cmif_types.h"
#include "core/hle/service/service.h"

namespace Service::NS {

class ISystemUpdateControl final : public ServiceFramework<ISystemUpdateControl> {
public:
    explicit ISystemUpdateControl(Core::System& system_);
    ~ISystemUpdateControl() override;

private:
    void SetupToReceiveSystemUpdate(HLERequestContext& ctx);
    void RequestCheckLatestUpdateIncludesRebootlessUpdate(HLERequestContext& ctx);
};

} // namespace Service::NS
