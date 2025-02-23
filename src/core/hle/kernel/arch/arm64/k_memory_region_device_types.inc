// SPDX-FileCopyrightText: Copyright 2021 yuzu Emulator Project & 2025 citron Homebrew Project
// SPDX-License-Identifier: GPL-2.0-or-later

// All architectures must define NumArchitectureDeviceRegions.
constexpr inline const auto NumArchitectureDeviceRegions = 3;

constexpr inline const auto KMemoryRegionType_Uart =
    KMemoryRegionType_ArchDeviceBase.DeriveSparse(0, NumArchitectureDeviceRegions, 0);
constexpr inline const auto KMemoryRegionType_InterruptCpuInterface =
    KMemoryRegionType_ArchDeviceBase.DeriveSparse(0, NumArchitectureDeviceRegions, 1)
        .SetAttribute(KMemoryRegionAttr_NoUserMap);
constexpr inline const auto KMemoryRegionType_InterruptDistributor =
    KMemoryRegionType_ArchDeviceBase.DeriveSparse(0, NumArchitectureDeviceRegions, 2)
        .SetAttribute(KMemoryRegionAttr_NoUserMap);
static_assert(KMemoryRegionType_Uart.GetValue() == (0x1D));
static_assert(KMemoryRegionType_InterruptCpuInterface.GetValue() ==
              (0x2D | KMemoryRegionAttr_NoUserMap));
static_assert(KMemoryRegionType_InterruptDistributor.GetValue() ==
              (0x4D | KMemoryRegionAttr_NoUserMap));
