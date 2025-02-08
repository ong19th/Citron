// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-FileCopyrightText: Copyright 2025 citron Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <span>
#include <vector>
#include <SDL.h>

#include "audio_core/common/common.h"
#include "audio_core/sink/sdl2_sink.h"
#include "audio_core/sink/sink_stream.h"
#include "common/logging/log.h"
#include "common/scope_exit.h"
#include "core/core.h"

namespace AudioCore::Sink {
/**
 * SDL sink stream, responsible for sinking samples to hardware.
 */
// class SDLSinkStream final : public SinkStream {
// ...
// }

SDLSink::SDLSink(std::string_view target_device_name) {
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_CRITICAL(Audio_Sink, "SDL_InitSubSystem audio failed: {}", SDL_GetError());
            return;
        }
    }

    if (target_device_name != auto_device_name && !target_device_name.empty()) {
        output_device = target_device_name;
    } else {
        output_device.clear();
    }

    device_channels = 2;
}

SDLSink::~SDLSink() = default;

SinkStream* SDLSink::AcquireSinkStream(Core::System& system, u32 system_channels_,
                                       const std::string&, StreamType type) {
    system_channels = system_channels_;
    SinkStreamPtr& stream = sink_streams.emplace_back(std::make_unique<SDLSinkStream>(
        device_channels, system_channels, output_device, input_device, type, system));
    return stream.get();
}

void SDLSink::CloseStream(SinkStream* stream) {
    for (size_t i = 0; i < sink_streams.size(); i++) {
        if (sink_streams[i].get() == stream) {
            sink_streams[i].reset();
            sink_streams.erase(sink_streams.begin() + i);
            break;
        }
    }
}

void SDLSink::CloseStreams() {
    sink_streams.clear();
}

f32 SDLSink::GetDeviceVolume() const {
    if (sink_streams.empty()) {
        return 1.0f;
    }

    return sink_streams[0]->GetDeviceVolume();
}

void SDLSink::SetDeviceVolume(f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetDeviceVolume(volume);
    }
}

void SDLSink::SetSystemVolume(f32 volume) {
    for (auto& stream : sink_streams) {
        stream->SetSystemVolume(volume);
    }
}

std::vector<std::string> ListSDLSinkDevices(bool capture) {
    std::vector<std::string> device_list;

    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_CRITICAL(Audio_Sink, "SDL_InitSubSystem audio failed: {}", SDL_GetError());
            return {};
        }
    }

    const int device_count = SDL_GetNumAudioDevices(capture);
    for (int i = 0; i < device_count; ++i) {
        if (const char* name = SDL_GetAudioDeviceName(i, capture)) {
            device_list.emplace_back(name);
        }
    }

    return device_list;
}

bool IsSDLSuitable() {
#if !defined(HAVE_SDL2)
    return false;
#else
    // Check SDL can init
    if (!SDL_WasInit(SDL_INIT_AUDIO)) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            LOG_ERROR(Audio_Sink, "SDL failed to init, it is not suitable. Error: {}",
                      SDL_GetError());
            return false;
        }
    }

    // We can set any latency frequency we want with SDL, so no need to check that.

    // Check we can open a device with standard parameters
    SDL_AudioSpec spec;
    spec.freq = TargetSampleRate;
    spec.channels = 2u;
    spec.format = AUDIO_S16SYS;
    spec.samples = TargetSampleCount * 2;
    spec.callback = nullptr;
    spec.userdata = nullptr;

    SDL_AudioSpec obtained;
    auto device = SDL_OpenAudioDevice(nullptr, false, &spec, &obtained, false);

    if (device == 0) {
        LOG_ERROR(Audio_Sink, "SDL failed to open a device, it is not suitable. Error: {}",
                  SDL_GetError());
        return false;
    }

    SDL_CloseAudioDevice(device);
    return true;
#endif
}

SDLSinkStream::SDLSinkStream(u32 device_channels_, u32 system_channels_, const std::string& output_device,
                            const std::string& input_device, StreamType type_, Core::System& system_)
    : SinkStream{system_, type_} {
    system_channels = system_channels_;
    device_channels = device_channels_;

    // Add error checking for SDL audio device
    if (SDL_GetNumAudioDevices(0) < 1) {
        LOG_ERROR(Service_Audio, "No audio devices available");
        return;
    }

    SDL_AudioSpec want{};
    want.freq = TargetSampleRate;
    want.format = AUDIO_S16LSB;
    want.channels = static_cast<u8>(std::min(device_channels, 2u)); // Limit to stereo
    want.samples = TargetSampleCount * 2; // Match the sample count from logs
    want.callback = nullptr;

    SDL_AudioSpec got;
    audio_device_id = SDL_OpenAudioDevice(nullptr, 0, &want, &got,
                                        SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);

    if (audio_device_id == 0) {
        LOG_ERROR(Service_Audio, "SDL_OpenAudioDevice failed: {}", SDL_GetError());
        return;
    }

    // Verify we got the requested format
    if (got.format != want.format || got.channels != want.channels) {
        LOG_WARNING(Service_Audio,
            "SDL audio format mismatch - wanted: {} ch, got: {} ch",
            want.channels, got.channels);
    }

    running = true;
    SDL_PauseAudioDevice(audio_device_id, 0);
}

SDLSinkStream::~SDLSinkStream() {
    LOG_DEBUG(Service_Audio, "Destructing SDL stream {}", name);
    Finalize();
}

void SDLSinkStream::Finalize() {
    if (audio_device_id == 0) {
        return;
    }

    Stop();
    SDL_ClearQueuedAudio(audio_device_id);
    SDL_CloseAudioDevice(audio_device_id);
}

void SDLSinkStream::Start(bool resume) {
    if (audio_device_id == 0 || !paused) {
        return;
    }

    paused = false;
    SDL_PauseAudioDevice(audio_device_id, 0);
}

void SDLSinkStream::Stop() {
    if (audio_device_id == 0 || paused) {
        return;
    }
    SignalPause();
    SDL_PauseAudioDevice(audio_device_id, 1);
}

void SDLSinkStream::DataCallback(void* userdata, Uint8* stream, int len) {
    auto* impl = static_cast<SDLSinkStream*>(userdata);

    if (!impl) {
        return;
    }

    const std::size_t num_channels = impl->GetDeviceChannels();
    const std::size_t frame_size = num_channels;
    const std::size_t num_frames{len / num_channels / sizeof(s16)};

    if (impl->type == StreamType::In) {
        std::span<const s16> input_buffer{reinterpret_cast<const s16*>(stream),
                                          num_frames * frame_size};
        impl->ProcessAudioIn(input_buffer, num_frames);
    } else {
        std::span<s16> output_buffer{reinterpret_cast<s16*>(stream), num_frames * frame_size};
        impl->ProcessAudioOutAndRender(output_buffer, num_frames);
    }
}

} // namespace AudioCore::Sink
