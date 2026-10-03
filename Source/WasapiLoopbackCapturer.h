// ============================================================================
//  WasapiLoopbackCapturer.h - capture whatever Windows is playing
//
//  Talks to the Windows Core Audio APIs directly (IMMDeviceEnumerator +
//  IAudioClient in AUDCLNT_STREAMFLAGS_LOOPBACK mode on the default render
//  endpoint). Runs its own thread; delivers interleaved->planar float stereo.
//  Windows-only; compiles to an empty stub elsewhere.
// ============================================================================
#pragma once

#include <JuceHeader.h>
#include <atomic>
#include <functional>
#include <vector>

#if JUCE_WINDOWS

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <avrt.h>

class WasapiLoopbackCapturer
{
public:
    using Callback = std::function<void (const float* left, const float* right,
                                         int numSamples, double sourceRate)>;

    WasapiLoopbackCapturer() = default;
    ~WasapiLoopbackCapturer() { stop(); }

    bool start (Callback cb)
    {
        if (thread.joinable())
            return true;
        callback = std::move (cb);
        shouldExit = false;
        thread = std::thread ([this] { run(); });
        return true;
    }

    void stop()
    {
        shouldExit = true;
        if (thread.joinable())
            thread.join();
        callback = nullptr;
    }

    double getSourceRate() const noexcept { return sourceRate.load(); }
    bool   isRunning() const noexcept     { return running.load(); }

private:
    void run()
    {
        // MMCSS: push this thread into the "Pro Audio" latency bucket
        DWORD mmcssTaskIndex = 0;
        HANDLE mmcss = AvSetMmThreadCharacteristicsW (L"Pro Audio", &mmcssTaskIndex);

        if (FAILED (CoInitializeEx (nullptr, COINIT_MULTITHREADED)))
        {
            if (mmcss) AvRevertMmThreadCharacteristics (mmcss);
            return;
        }

        IMMDeviceEnumerator* enumerator = nullptr;
        IMMDevice* device = nullptr;
        IAudioClient* client = nullptr;
        IAudioCaptureClient* capture = nullptr;
        WAVEFORMATEX* mixFormat = nullptr;

        if (SUCCEEDED (CoCreateInstance (__uuidof (MMDeviceEnumerator), nullptr,
                                         CLSCTX_ALL, IID_PPV_ARGS (&enumerator))))
            if (SUCCEEDED (enumerator->GetDefaultAudioEndpoint (eRender, eConsole, &device)))
                if (SUCCEEDED (device->Activate (__uuidof (IAudioClient), CLSCTX_ALL,
                                                 nullptr, (void**) &client)))
                    if (SUCCEEDED (client->GetMixFormat (&mixFormat)))
                    {
                        sourceRate = mixFormat->nSamplesPerSec;

                        // 200 ms shared-mode buffer, loopback of the render stream
                        const REFERENCE_TIME bufferHns = 200 * 10000;
                        if (SUCCEEDED (client->Initialize (AUDCLNT_SHAREMODE_SHARED,
                                                           AUDCLNT_STREAMFLAGS_LOOPBACK,
                                                           bufferHns, 0, mixFormat, nullptr)))
                            if (SUCCEEDED (client->GetService (IID_PPV_ARGS (&capture))))
                            {
                                client->Start();
                                running = true;
                                pumpLoop (capture, mixFormat);
                                running = false;
                                client->Stop();
                            }
                    }

        if (capture)   capture->Release();
        if (client)    client->Release();
        if (mixFormat) CoTaskMemFree (mixFormat);
        if (device)    device->Release();
        if (enumerator) enumerator->Release();
        CoUninitialize();
        if (mmcss) AvRevertMmThreadCharacteristics (mmcss);
    }

    void pumpLoop (IAudioCaptureClient* capture, WAVEFORMATEX* fmt)
    {
        const int channels = (int) fmt->nChannels;
        const bool isFloat = (fmt->wFormatTag == WAVE_FORMAT_IEEE_FLOAT)
                          || (fmt->wFormatTag == WAVE_FORMAT_EXTENSIBLE);
        const int bytesPerSample = (int) fmt->wBitsPerSample / 8;

        while (! shouldExit)
        {
            Sleep (4);   // poll cadence (~4 ms) - simple and robust for loopback

            UINT32 packetFrames = 0;
            while (SUCCEEDED (capture->GetNextPacketSize (&packetFrames)) && packetFrames > 0)
            {
                BYTE* data = nullptr;
                UINT32 numFrames = 0;
                DWORD flags = 0;

                if (FAILED (capture->GetBuffer (&data, &numFrames, &flags, nullptr, nullptr)))
                    break;

                if (numFrames > 0)
                {
                    const int n = (int) numFrames;
                    leftBuf.resize ((size_t) n);
                    rightBuf.resize ((size_t) n);

                    const bool silent = (flags & AUDCLNT_BUFFERFLAGS_SILENT) != 0;
                    if (silent || data == nullptr)
                    {
                        std::fill (leftBuf.begin(), leftBuf.end(), 0.0f);
                        std::fill (rightBuf.begin(), rightBuf.end(), 0.0f);
                    }
                    else if (isFloat)
                    {
                        const float* s = reinterpret_cast<const float*> (data);
                        for (int i = 0; i < n; ++i)
                        {
                            leftBuf[(size_t) i]  = s[(size_t) (i * channels)];
                            rightBuf[(size_t) i] = channels > 1 ? s[(size_t) (i * channels + 1)]
                                                                : s[(size_t) (i * channels)];
                        }
                    }
                    else   // 16-bit PCM
                    {
                        const short* s = reinterpret_cast<const short*> (data);
                        for (int i = 0; i < n; ++i)
                        {
                            leftBuf[(size_t) i]  = (float) s[(size_t) (i * channels)]     / 32768.0f;
                            rightBuf[(size_t) i] = channels > 1 ? (float) s[(size_t) (i * channels + 1)] / 32768.0f
                                                                : leftBuf[(size_t) i];
                        }
                    }

                    capture->ReleaseBuffer (numFrames);

                    if (callback)
                        callback (leftBuf.data(), rightBuf.data(), n, sourceRate.load());
                }
                else
                {
                    capture->ReleaseBuffer (numFrames);
                }
            }
        }
        (void) bytesPerSample;
    }

    Callback callback;
    std::thread thread;
    std::atomic<bool> shouldExit { false };
    std::atomic<bool> running { false };
    std::atomic<double> sourceRate { 48000.0 };
    std::vector<float> leftBuf, rightBuf;
};

#else   // ---------------------------------------------------------------- stub

class WasapiLoopbackCapturer
{
public:
    using Callback = std::function<void (const float*, const float*, int, double)>;
    bool start (Callback) { return false; }
    void stop() {}
    double getSourceRate() const noexcept { return 48000.0; }
    bool isRunning() const noexcept { return false; }
};

#endif
