# AudioFX — System-Wide Audio FX for Windows

**10-Band Equalizer · Live Spectrum · Reverb · Field Surround · Compressor ·
Look-ahead Limiter · Master Volume with Auto Level (AGC)**

Built with **JUCE 8** and **Visual Studio 2026** (v145 toolset) · modern dark-glass GUI ·
**VB-Cable** system capture (with native **WASAPI loopback** fallback) · validated, unit-tested DSP core.

```
┌────────────────────────────────────────────────────────────────────────────┐
│  WINDOWS AUDIO  (games, browser, Spotify, DAW …)                           │
└──────────────┬─────────────────────────────────────────────────────────────┘
               │  default playback device = "CABLE Input (VB-Audio Virtual Cable)"
               ▼
┌──────────────────────┐    capture     ┌────────────────────────────────────┐
│ VB-Cable virtual     │───────────────▶│            AudioFX                 │
│ cable (kernel drv)   │  "CABLE Output"│  ┌──────────┐  ┌────────┐          │
└──────────────────────┘                │  │ 10-Band │─▶│ Reverb │─┐        │
   - or -                               │  │   EQ    │  └────────┘ │        │
┌──────────────────────┐  WASAPI        │  └──────────┘             ▼        │
│ Default render       │──loopback─────▶│  ┌──────────┐  ┌────────────────┐ │
│ endpoint ("what you  │  (no driver)   │  │  Field   │─▶│ Master + AGC   │ │
│  hear")              │                │  │ Surround │  │ (level keeper) │ │
└──────────────────────┘                │  └──────────┘  └───────┬────────┘ │
                                        │                  ┌─────▼───────┐  │
                                        │                  │  Look-ahead │  │
                                        │                  │   Limiter   │  │
                                        │                  └─────┬───────┘  │
                                        └────────────────────────┼──────────┘
                                                                 ▼
                                                     Speakers / Headphones
```

---

## Feature map

| Module | What it does | Controls |
|---|---|---|
| **10-Band EQ** | ISO-octave graphic EQ (31 Hz … 16 kHz), shelving ends + peaking mids, smoothed parameter transitions, live composite response curve | ±15 dB per band, FLAT reset, per-module bypass |
| **Spectrum** | 4096-point radix-2 FFT (Hann, 50 % overlap), log-frequency display with peak-hold | — (auto) |
| **Reverb** | Schroeder tank: 8 damped combs + 4 true allpasses per channel, stereo spread | Room, Damp, Wet, Width, Freeze |
| **Field Surround** | Virtual surround field: mid/side spread, decorrelated pseudo-side diffusion (widens mono too), Haas micro-delay, bass-focus mono anchoring | Field, Spread, Bass, Center |
| **Compressor** | Programme compressor: peak envelope detector, 6 dB soft knee, ratio 1–20:1, 5 ms grab / 120 ms recovery ballistics, makeup gain, GR meter | Threshold, Ratio, Attack, Release, Makeup |
| **Limiter** | True look-ahead peak limiter (5 ms): sliding-min gain computer, linear attack ramp, one-pole release, hard-guaranteed ceiling | Ceiling (−24…0 dB), Release, GR meter |
| **Master** | Master volume + **Auto Level (AGC)** that *maintains output loudness* at a target dBFS (bounded ±12 dB, 800 ms/2.5 s ballistics) into the limiter | Volume (−60…+12 dB), AUTO LEVEL, Target |

Extra: per-module bypass, global bit-transparent bypass, factory + user presets (XML),
settings/device persistence, stereo-linked processing everywhere.

---

## Repository layout (the "full stack")

```
AudioFX/
├── CMakeLists.txt            JUCE 8.0.12 (FetchContent) → Visual Studio 18 (2026)
├── build.ps1 / build.bat     one-shot configure + build (+ optional tests)
├── dsp/                      ★ pure C++17 DSP core (zero JUCE, unit-tested)
│   ├── DspCommon.h           biquads (RBJ), delay lines, smoothers
│   ├── TenBandEQ             ReverbEngine        FieldSurround
│   ├── LookaheadLimiter      MasterBus (AGC)     SpectrumFft
│   ├── AudioFifo             clock-drift servo (capture ⇄ render)
│   └── ProcessingChain       EQ→Reverb→Surround→Comp→Master→Limiter + meters
├── tests/dsp_tests.cpp       36 numeric validation tests (runs anywhere g++ exists)
├── Source/                   JUCE application layer
│   ├── Main.cpp              app + native window/icon
│   ├── AudioEngine.*         device I/O ⇄ DSP bridge (capture modes)
│   ├── WasapiLoopbackCapturer.h  raw WASAPI "what you hear" capture thread
│   ├── State/Params.h        parameter model + XML serialise + apply
│   ├── State/PresetManager.* factory/user presets + settings persistence
│   └── UI/                   dark-glass GUI (Theme, LookAndFeel, panels)
├── Scripts/
│   ├── Install-VBCable.ps1       silent VB-Cable driver install (admin)
│   ├── Set-DefaultAudioDevice.ps1 route all audio ⇄ cable / restore speakers
│   ├── Register-AudioFX-Autostart.ps1  start with Windows
│   └── WinAudio.cs               MMDevice + IPolicyConfig interop
├── Installer/Setup.iss       Inno Setup packaging (optional)
└── resources/                logo.png + icon.ico (generated branding)
```

**Signal path (per block):**
`capture → 10-Band EQ → Reverb → Field Surround → Compressor → Master Volume+AGC → Look-ahead Limiter → render`
The spectrum taps the post-EQ signal so the analyser shows what the faders do.

---

## Build (Windows 10/11 x64)

Prerequisites: **Visual Studio 2026** with *Desktop development with C++* (v145),
**CMake ≥ 4.2** (VS 2026 Update 4 ships 4.2.3 — earlier updates can use the bundled
VS-CMake or any CMake ≥ 3.22 with the default generator), internet on first
configure (JUCE is fetched automatically), and optionally **g++/clang/MSVC** for the
DSP tests (any C++17 compiler).

```powershell
# 1. configure + build (picks "Visual Studio 18 2026" / x64 automatically)
powershell -ExecutionPolicy Bypass -File build.ps1

# 2. optional: validate the DSP core while you're at it
powershell -ExecutionPolicy Bypass -File build.ps1 -Tests

# result: dist\AudioFX.exe   (build\AudioFX_artefacts\Release\AudioFX.exe)
```

Or by hand:

```powershell
cmake -B build -G "Visual Studio 18 2026" -A x64
cmake --build build --config Release
```

You can also open `build\AudioFX.sln` in Visual Studio 2026 and press F5.
(If your CMake predates 4.2, drop the `-G` flag — the VS 2026 default generator is used.)

### DSP core validation (works on any OS)

```sh
sh tests/run_tests.sh          # Linux/macOS/MSYS
```

36 checks: EQ band accuracy (+12 dB at 1 kHz measured +11.93 dB), reverb decay &
boundedness, surround decorrelation + bass anchoring, limiter ceiling guarantee under
+12 dBFS abuse and impulse trains, AGC convergence to −18 dBFS ±1 dB, FFT bin
accuracy, clock-drift FIFO servoing (44.1 ⇄ 48 kHz), chain stress (2000 random
blocks never exceed the ceiling), bit-transparent bypass.

---

## Setup — system-wide audio with VB-Cable

```powershell
# 1. install the virtual cable (elevated; REBOOT afterwards)
powershell -ExecutionPolicy Bypass -File Scripts\Install-VBCable.ps1

# 2. route ALL Windows audio into the cable (elevated not required)
powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1

# 3. start AudioFX, pick your real speakers/headphones as OUTPUT
#    (capture = "CABLE Output (VB-Audio Virtual Cable)" is auto-detected)

# later, when you want plain audio again:
powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1 -Restore
```

Or let the **installer** (`iscc Installer\Setup.iss`) do steps 1–2 for you.

**No VB-Cable?** Set the capture mode to **WASAPI LOOPBACK** in the top bar — AudioFX
then grabs the default render endpoint directly with the Windows Core Audio API
(`AUDCLNT_STREAMFLAGS_LOOPBACK`), resampling/rate-servoing as needed. Latency is a
little higher than the cable path but no driver install is required.

### Capture modes at a glance

| | VB-Cable / Device | WASAPI Loopback |
|---|---|---|
| Install needed | VB-Cable driver + reboot | none |
| Latency | device buffer (~10–25 ms) | ~85 ms FIFO + poll |
| Clock domains | one (inline processing) | two (adaptive resampler + proportional rate servo) |
| Use case | daily driver, games, media | quick try / no-admin machines |

---

## GUI tour

* **Top bar** — capture mode, input (auto-picks *CABLE Output*), output device,
  preset browser (SAVE/DEL), global BYPASS, live status (sample rate / block size).
* **Spectrum** — log-frequency FFT with peak-hold, grid at 50 Hz–10 kHz / 12 dB steps.
* **10-Band Equalizer** — drag knobs (double-click = 0 dB), live response curve
  sweeps behind the controls, FLAT button.
* **Reverb** — Room / Damp / Wet / Width + FREEZE (infinite sustain).
* **Field Surround** — live field visualiser (ellipse = spread, glow = field amount,
  magenta core = bass anchor), Field / Spread / Bass / Center.
* **Compressor** — Threshold / Ratio / Attack / Release / Makeup + GR meter; lives in
  its own tab (REVERB | FIELD | COMP | LIMITER) in the FX row.
* **Limiter** — Ceiling / Release + gain-reduction meter and dB read-out.
* **Master** — big volume knob, AUTO LEVEL with target loudness, stereo meters and
  programme level / AGC-gain read-out — **output level is maintained** into the
  limiter's hard ceiling.

Presets: `Flat, Bass Boost, Vocal Clarity, Cinema Field, Night Mode, Studio Ref`
plus your own (stored in `%APPDATA%\AudioFX\Presets`). Settings & device routing
persist in `%APPDATA%\AudioFX\settings.xml`.

---

## Engineering notes

* **DSP core is JUCE-free** and compiled/tested independently (`dsp/`, `tests/`) —
  the GUI/audio shells can change without touching the maths.
* **Limiter contract:** `peak(out) ≤ ceiling`, guaranteed twice over — linear attack
  ramp exactly one look-ahead window long against a sliding-min gain target, plus a
  final clamp. Verified by tests under +12 dBFS sines and ±3.0 noise.
* **AGC** measures pre-fader programme level (50 ms/500 ms RMS ballistics) and
  drives gain toward the target with 800 ms down / 2.5 s up time constants, clamped
  to ±12 dB so it never "pumps" or fights the mix.
* **Rate servo:** in loopback mode the capture clock and render clock drift apart
  (two physical oscillators). The FIFO's proportional servo nudges the resampling
  ratio ±0.4 % to keep the buffer level-locked — pitch shift is inaudible.
* **Threading:** capture thread (WASAPI) → lock-free-ish FIFO → audio callback
  (DSP) → atomics → 30 Hz GUI timer. Parameter writes are message-thread →
  audio-thread double/float updates with internal smoothing (zipper-free).
* All DSP runs in `float`, stereo-linked (mid-linked limiting), with NaN-safe
  clamps at the chain output.

## Troubleshooting

| Symptom | Fix |
|---|---|
| No sound after install | Reboot after VB-Cable install; run `Set-DefaultAudioDevice.ps1 -Restore` to get speakers back |
| AudioFX hears nothing | Windows *Settings → System → Sound → Output* must be **CABLE Input**; capture device = **CABLE Output** |
| Crackles in loopback mode | raise the audio device buffer (top-bar status shows the block size), or switch to the VB-Cable path |
| Duplicate/crackling echo | make sure the output device in AudioFX is the *physical* device, and Windows' default output is the cable (not both on the cable) |
| CMake generator error | update CMake/VS 2026, or configure without `-G` |

## Licensing

* Code in this repository: use freely.
* **JUCE** is fetched at configure time and is subject to the
  [JUCE license](https://juce.com/legal/juce-8-licence/) (GPLv3 or commercial).
* **VB-Cable** is donationware by VB-Audio — separate terms.
