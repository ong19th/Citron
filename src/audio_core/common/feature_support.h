// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <map>
#include <ranges>
#include <tuple>

#include "common/assert.h"
#include "common/common_funcs.h"
#include "common/common_types.h"
#include "common/polyfill_ranges.h"

namespace AudioCore {
constexpr u32 CurrentRevision = 18;

enum class SupportTags {
    CommandProcessingTimeEstimatorVersion4,
    CommandProcessingTimeEstimatorVersion3,
    CommandProcessingTimeEstimatorVersion2,
    MultiTapBiquadFilterProcessing,
    EffectInfoVer2,
    WaveBufferVer2,
    BiquadFilterFloatProcessing,
    VolumeMixParameterPrecisionQ23,
    MixInParameterDirtyOnlyUpdate,
    BiquadFilterEffectStateClearBugFix,
    VoicePlayedSampleCountResetAtLoopPoint,
    VoicePitchAndSrcSkipped,
    SplitterBugFix,
    FlushVoiceWaveBuffers,
    ElapsedFrameCount,
    AudioRendererVariadicCommandBufferSize,
    PerformanceMetricsDataFormatVersion2,
    AudioRendererProcessingTimeLimit80Percent,
    AudioRendererProcessingTimeLimit75Percent,
    AudioRendererProcessingTimeLimit70Percent,
    AdpcmLoopContextBugFix,
    Splitter,
    LongSizePreDelay,
    AudioUsbDeviceOutput,
    DeviceApiVersion2,
    DelayChannelMappingChange,
    ReverbChannelMappingChange,
    I3dl2ReverbChannelMappingChange,
    AudioOutAuto,
    AudioInAuto,
    AudioInFiltering,
    AudioInUacSupport,
    FinalOutputRecorderAuto,
    FinalOutputRecorderWorkBuffer,
    AudioRendererManualExecution,
    AudioRendererVariableRate,
    AudioRendererRevisionCheck,
    AudioRendererVoiceDrop,
    AudioDeviceNameAuto,
    AudioDeviceInputEvent,
    AudioDeviceOutputEvent,
    AudioDeviceNotification,
    AudioDeviceAutoVolumeTune,
    HardwareOpusDecoderEx,
    HardwareOpusMultiStream,
    HardwareOpusLargeFrameSize,
    AudioSystemMasterVolume,
    AudioSystemInputVolume,
    AudioSystemRecordVolume,
    AudioSystemAutoMute,
    AudioSystemHearingProtection,
    AudioCompressorStatistics,
    AudioVolumeResetSupport,
    Size
};

constexpr u32 GetRevisionNum(u32 user_revision) {
    if (user_revision >= 0x100) {
        user_revision -= Common::MakeMagic('R', 'E', 'V', '0');
        user_revision >>= 24;
    }
    return user_revision;
};

constexpr bool CheckFeatureSupported(SupportTags tag, u32 user_revision) {
    constexpr std::array<std::pair<SupportTags, u32>, static_cast<u32>(SupportTags::Size)> features{
        {
            {SupportTags::AudioRendererProcessingTimeLimit70Percent, 1},
            {SupportTags::Splitter, 2},
            {SupportTags::AdpcmLoopContextBugFix, 2},
            {SupportTags::LongSizePreDelay, 3},
            {SupportTags::AudioUsbDeviceOutput, 4},
            {SupportTags::AudioRendererProcessingTimeLimit75Percent, 4},
            {SupportTags::VoicePlayedSampleCountResetAtLoopPoint, 5},
            {SupportTags::VoicePitchAndSrcSkipped, 5},
            {SupportTags::SplitterBugFix, 5},
            {SupportTags::FlushVoiceWaveBuffers, 5},
            {SupportTags::ElapsedFrameCount, 5},
            {SupportTags::AudioRendererProcessingTimeLimit80Percent, 5},
            {SupportTags::AudioRendererVariadicCommandBufferSize, 5},
            {SupportTags::PerformanceMetricsDataFormatVersion2, 5},
            {SupportTags::CommandProcessingTimeEstimatorVersion2, 5},
            {SupportTags::BiquadFilterEffectStateClearBugFix, 6},
            {SupportTags::BiquadFilterFloatProcessing, 7},
            {SupportTags::VolumeMixParameterPrecisionQ23, 7},
            {SupportTags::MixInParameterDirtyOnlyUpdate, 7},
            {SupportTags::WaveBufferVer2, 8},
            {SupportTags::CommandProcessingTimeEstimatorVersion3, 8},
            {SupportTags::EffectInfoVer2, 9},
            {SupportTags::CommandProcessingTimeEstimatorVersion4, 10},
            {SupportTags::MultiTapBiquadFilterProcessing, 10},
            {SupportTags::DelayChannelMappingChange, 11},
            {SupportTags::ReverbChannelMappingChange, 11},
            {SupportTags::I3dl2ReverbChannelMappingChange, 11},
            {SupportTags::AudioOutAuto, 3},
            {SupportTags::AudioInAuto, 3},
            {SupportTags::AudioInFiltering, 3},
            {SupportTags::AudioInUacSupport, 3},
            {SupportTags::FinalOutputRecorderAuto, 3},
            {SupportTags::FinalOutputRecorderWorkBuffer, 9},
            {SupportTags::AudioRendererManualExecution, 3},
            {SupportTags::AudioRendererVariableRate, 4},
            {SupportTags::AudioRendererRevisionCheck, 4},
            {SupportTags::AudioRendererVoiceDrop, 15},
            {SupportTags::AudioDeviceNameAuto, 3},
            {SupportTags::AudioDeviceInputEvent, 3},
            {SupportTags::AudioDeviceOutputEvent, 3},
            {SupportTags::AudioDeviceNotification, 17},
            {SupportTags::AudioDeviceAutoVolumeTune, 18},
            {SupportTags::HardwareOpusDecoderEx, 12},
            {SupportTags::HardwareOpusMultiStream, 3},
            {SupportTags::HardwareOpusLargeFrameSize, 12},
            {SupportTags::AudioSystemMasterVolume, 4},
            {SupportTags::AudioSystemInputVolume, 4},
            {SupportTags::AudioSystemRecordVolume, 4},
            {SupportTags::AudioSystemAutoMute, 13},
            {SupportTags::AudioSystemHearingProtection, 14},
            {SupportTags::AudioCompressorStatistics, 16},
            {SupportTags::AudioVolumeResetSupport, 17},
        }};

    const auto& feature =
        std::ranges::find_if(features, [tag](const auto& entry) { return entry.first == tag; });
    if (feature == features.cend()) {
        LOG_ERROR(Service_Audio, "Invalid SupportTag {}!", static_cast<u32>(tag));
        return false;
    }
    user_revision = GetRevisionNum(user_revision);
    return (*feature).second <= user_revision;
}

constexpr bool CheckValidRevision(u32 user_revision) {
    return GetRevisionNum(user_revision) <= CurrentRevision;
};

} // namespace AudioCore
