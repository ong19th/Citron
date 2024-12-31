// SPDX-FileCopyrightText: Copyright 2022 yuzu Emulator Project & 2025 citron Homebrew Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

#include "audio_core/common/common.h"
#include "common/common_types.h"

namespace AudioCore::ADSP::AudioRenderer {
class CommandListProcessor;
}

namespace AudioCore::Renderer {
using namespace ::AudioCore::ADSP;

enum class CommandId : u8 {
    /* 0x00 */ Invalid,
    /* 0x01 */ DataSourcePcmInt16Version1,
    /* 0x02 */ DataSourcePcmInt16Version2,
    /* 0x03 */ DataSourcePcmFloatVersion1,
    /* 0x04 */ DataSourcePcmFloatVersion2,
    /* 0x05 */ DataSourceAdpcmVersion1,
    /* 0x06 */ DataSourceAdpcmVersion2,
    /* 0x07 */ Volume,
    /* 0x08 */ VolumeRamp,
    /* 0x09 */ BiquadFilter,
    /* 0x0A */ Mix,
    /* 0x0B */ MixRamp,
    /* 0x0C */ MixRampGrouped,
    /* 0x0D */ DepopPrepare,
    /* 0x0E */ DepopForMixBuffers,
    /* 0x0F */ Delay,
    /* 0x10 */ Upsample,
    /* 0x11 */ DownMix6chTo2ch,
    /* 0x12 */ Aux,
    /* 0x13 */ DeviceSink,
    /* 0x14 */ CircularBufferSink,
    /* 0x15 */ Reverb,
    /* 0x16 */ I3dl2Reverb,
    /* 0x17 */ Performance,
    /* 0x18 */ ClearMixBuffer,
    /* 0x19 */ CopyMixBuffer,
    /* 0x1A */ LightLimiterVersion1,
    /* 0x1B */ LightLimiterVersion2,
    /* 0x1C */ MultiTapBiquadFilter,
    /* 0x1D */ Capture,
    /* 0x1E */ Compressor,
};

constexpr u32 CommandMagic{0xCAFEBABE};

/**
 * A command, generated by the host, and processed by the ADSP's AudioRenderer.
 */
struct ICommand {
    virtual ~ICommand() = default;

    /**
     * Print this command's information to a string.
     *
     * @param processor - The CommandListProcessor processing this command.
     * @param string    - The string to print into.
     */
    virtual void Dump(const AudioRenderer::CommandListProcessor& processor,
                      std::string& string) = 0;

    /**
     * Process this command.
     *
     * @param processor - The CommandListProcessor processing this command.
     */
    virtual void Process(const AudioRenderer::CommandListProcessor& processor) = 0;

    /**
     * Verify this command's data is valid.
     *
     * @param processor - The CommandListProcessor processing this command.
     * @return True if the command is valid, otherwise false.
     */
    virtual bool Verify(const AudioRenderer::CommandListProcessor& processor) = 0;

    /// Command magic 0xCAFEBABE
    u32 magic{};
    /// Command enabled
    bool enabled{};
    /// Type of this command (see CommandId)
    CommandId type{};
    /// Size of this command
    s16 size{};
    /// Estimated processing time for this command
    u32 estimated_process_time{};
    /// Node id of the voice or mix this command was generated from
    u32 node_id{};
};

} // namespace AudioCore::Renderer
