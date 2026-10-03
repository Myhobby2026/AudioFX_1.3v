# 🎛️ AudioFX — Setup + Build Guide (Hinglish)

> Ek jagah poora process: **Software install → Build → VB-Cable → Audio routing → App chalao**
> Sab commands copy-paste ready hain. Bas step-by-step follow karo.

---

## STEP 0 — Pehle ye sab install karo (Prerequisites)

| # | Software | Kyun chahiye | Kahan se milega |
|---|----------|--------------|-----------------|
| 1 | **Visual Studio 2026** (Community free hai) | C++ compiler + MSVC toolset (v145) | https://visualstudio.microsoft.com/downloads/ |
| 2 | **Git** | JUCE automatically download hota hai (FetchContent) | https://git-scm.org |
| 3 | **Internet** (pehli build ke time) | JUCE 8.0.12 GitHub se fetch hoga | — |
| 4 | *(Optional)* **g++ / MinGW** | DSP core ke tests chalane ke liye | MSYS2 ya TDM-GCC |

### VS 2026 install karte time ye workload TICK karo:
```
✔ Desktop development with C++     ← (ye MUST hai)
```
Right side "Installation details" mein ye bhi confirm karo:
```
✔ MSVC v145 - VS 2026 C++ x64/x86 build tools
✔ Windows 11 SDK (koi bhi latest)
✔ C++ CMake tools for Windows      ← (VS ke andar CMake mil jayega)
```
> 💡 **Tip:** VS 2026 (18.x) ke saath CMake 4.2.3+ aata hai — usme
> `"Visual Studio 18 2026"` generator ready milta hai. Agar CMake purana hai
> to koi tension nahi — hamari `build.ps1` khud fallback kar deti hai.

---

## STEP 1 — Project folder ready karo

Poora `AudioFX` folder apne PC pe rakh lo (e.g. `D:\Projects\AudioFX`).
Andar ye hona chahiye:
```
AudioFX\
├── build.ps1          ← MAIN build script (ise hi chalana hai)
├── CMakeLists.txt
├── dsp\               ← DSP core (tested maths)
├── Source\            ← JUCE app + GUI
├── Scripts\           ← VB-Cable + audio routing tools
├── tests\             ← DSP tests
├── Installer\         ← (optional) installer script
└── resources\         ← logo + icon
```

---

## STEP 2 — Build karo (2 tarike hain)

### 🌟 Tarika A — Easy (recommended)

**PowerShell kholo** (Start menu mein "PowerShell" search karo →
**"Run as Administrator" zaroori NAHI** for build):

```powershell
cd D:\Projects\AudioFX
powershell -ExecutionPolicy Bypass -File build.ps1
```

**Bas!** Ye script khud:
1. VS 2026 dhundhti hai (`vswhere` se)
2. CMake configure karti hai — generator `"Visual Studio 18 2026"` / x64
3. JUCE 8.0.12 automatically download + build hota hai (pehli baar 2–5 min lagega)
4. Release build banata hai
5. Output copy karti hai → **`D:\Projects\AudioFX\dist\AudioFX.exe`** ✅

Agar beech mein kuch download/type ho to ghabrao mat — normal hai.

**Result dekhne ke liye:**
```powershell
.\dist\AudioFX.exe
```

### 🔧 Tarika B — Manual (khud control chahiye to)

```powershell
cd D:\Projects\AudioFX

# 1. Configure (project files generate honge + JUCE download)
cmake -B build -G "Visual Studio 18 2026" -A x64

# 2. Build
cmake --build build --config Release
```

Executable yahan milega:
```
build\AudioFX_artefacts\Release\AudioFX.exe
```

> 📌 Agar `-G "Visual Studio 18 2026"` wala error aaye (CMake purana hai), to
> bas `-G ...` hata do:
> ```powershell
> cmake -B build -A x64
> ```
> CMake khud VS 2026 default generator le lega.

**Visual Studio IDE se bhi chalana ho to:**
`build\AudioFX.sln` double-click karo → **F5** dabao. Ho gaya.

### 🧪 (Optional) DSP tests chalao — maths verify karne ke liye

Agar g++ installed hai:
```powershell
cd D:\Projects\AudioFX
powershell -ExecutionPolicy Bypass -File build.ps1 -Tests
```
Ya direct:
```powershell
g++ -std=c++17 -O2 tests\dsp_tests.cpp -o build\dsp_tests.exe -I .
.\build\dsp_tests.exe
```
End mein **"ALL TESTS PASSED (0 failures)"** dikhna chahiye. 🎉

---

## STEP 3 — VB-Cable install karo (system-wide audio ke liye)

Ye virtual cable hai — iske through **saara Windows audio** (game, browser,
Spotify, sab) AudioFX mein jayega.

> ⚠️ **Ye step ADMIN (Run as Administrator) se karna hai.**

### 3a. Installer chalao
```powershell
# PEHLE PowerShell ko Admin kholo (Start → PowerShell → Right-click → Run as Administrator)
cd D:\Projects\AudioFX
powershell -ExecutionPolicy Bypass -File Scripts\Install-VBCable.ps1
```

Ye script khud:
1. VB-Cable download karti hai (v45 pack, official site se)
2. `VBCABLE_Setup_x64.exe -i` se silent install karti hai
3. Done ka message deti hai

### 3b. PC REBOOT karo 🔁
> **Bina reboot ke cable nahi dikhega.** Ye driver-level cheez hai —
> restart zaroori hai.

### 3c. Verify karo (reboot ke baad)
Settings → **System → Sound** → Output list mein ye dikhna chahiye:
```
CABLE Input (VB-Audio Virtual Cable)     ← playback side
```
Aur Input list mein:
```
CABLE Output (VB-Audio Virtual Cable)    ← capture side
```

---

## STEP 4 — Audio routing setup karo

Ab saara Windows audio cable mein bhejna hai:

```powershell
powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1
```

Ye script **default playback device = "CABLE Input"** set kar deti hai
(teeno roles: Console / Multimedia / Communications).

**Kaise samjhein ki ho gaya:**
```
Windows Apps (Spotify/game/browser)
        │
        ▼
[ CABLE Input ] ──virtual cable──▶ [ CABLE Output ] ──▶ AudioFX ──▶ Speakers
```

> 🔙 **Baad mein normal audio wapas chahiye?** Ye chalao:
> ```powershell
> powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1 -Restore
> ```
> Desktop shortcut banane ke liye: `Scripts\Register-AudioFX-Autostart.ps1` (app startup pe khud chalu).

---

## STEP 5 — AudioFX app setup (pehli baar)

1. **`dist\AudioFX.exe` double-click** karo 🚀
2. Top bar check karo:
   ```
   [ VB-CABLE / DEVICE ▾ ] [ CABLE Output (VB-Audio…) ▾ ] [ Speakers/Headphones ▾ ]
   ```
   - **Capture mode:** `VB-CABLE / DEVICE`
   - **Input (capture):** `CABLE Output (VB-Audio Virtual Cable)` ← auto-detect hota hai
   - **Output:** apne asli speakers / headphones
3. Koi bhi music/chrome tab play karo → **Spectrum hilna chahiye** 📊
4. Ab maze lo:
   - **EQ** — 10 knobs drag (double-click = 0 dB, FLAT = reset)
   - **REVERB** — Room/Wet badho
   - **FIELD SURROUND** — Field + Spread (visualization live dikhega)
   - **MASTER** — volume knob + **AUTO LEVEL** on rakho → output level
     khud maintain hoga (limiter ceiling ke andar)
5. Settings apne aap save hoti hain (`%APPDATA%\AudioFX\`)

### ❓ VB-Cable nahi install karna?
Capture mode mein **`WASAPI LOOPBACK`** choose karo — bina kisi driver ke
"what you hear" capture ho jayega (thoda latency zyada hota hai).

---

## 🆘 Problem? (Troubleshooting — Hinglish)

| Problem | Solution |
|---------|----------|
| Build mein "generator not found" error | `cmake -B build -A x64` (bina `-G`) chalao |
| **"Could not find icon file ... icon.ico"** | JUCE bug — CMakeLists mein `ICON_BIG`/`ICON_SMALL` ko `.png` hona chahiye (`.ico` nahi). Naya CMakeLists.txt use karo (already fixed) |
| `JuceHeader.h: No such file or directory` | CMakeLists mein `juce_generate_juce_header(AudioFX)` line honi chahiye (already fixed) |
| JUCE download fail | Internet check karo; ya proxy/company firewall; phir se `build.ps1` |
| `BinaryData.h not found` error | `resources\logo.png` missing hai — check karo folder |
| App chali but **awaaz nahi aa rahi** | Step 4 dobara — default output `CABLE Input` hona chahiye |
| Music sunai de raha hai but **Spectrum nahi hilta** | Input mein `CABLE Output` select hai? Mode `VB-CABLE / DEVICE` hai? |
| Crackles / tuk-tuk awaaz | Loopback mode hai to buffer badhao; warna VB-Cable wala mode use karo (zyada stable) |
| Double awaaz / echo | AudioFX ka **output** physical speakers rakho, Windows default **cable** — dono cable mat rakhna |
| VB-Cable ke baad device nahi dikha | **Reboot** kiya? Reboot ke baad bhi na dikhe to installer admin se dobara |
| Sab wapas normal karna hai | `Set-DefaultAudioDevice.ps1 -Restore` |
| App start pe khud na chale | `Scripts\Register-AudioFX-Autostart.ps1` chalao |

---

## ⚡ Quick Command Cheat-Sheet

```powershell
# ── BUILD ─────────────────────────────────────────────
cd D:\Projects\AudioFX
powershell -ExecutionPolicy Bypass -File build.ps1          # build
powershell -ExecutionPolicy Bypass -File build.ps1 -Tests   # build + tests

# ── SETUP (Admin PowerShell) ─────────────────────────
powershell -ExecutionPolicy Bypass -File Scripts\Install-VBCable.ps1
# --- PC RESTART karo ---
powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1

# ── CHALAO ───────────────────────────────────────────
.\dist\AudioFX.exe

# ── WAPAS NORMAL AUDIO ───────────────────────────────
powershell -ExecutionPolicy Bypass -File Scripts\Set-DefaultAudioDevice.ps1 -Restore
```

---

## Order yaad rakho (One-liner):

> **VS 2026 install → `build.ps1` → `Install-VBCable.ps1` → REBOOT →
> `Set-DefaultAudioDevice.ps1` → `dist\AudioFX.exe` → Music on → EQ lo!** 🎧

Bas itna hi. Ab build karke chalao — koi error aaye to wahi error message bhej
do, main fix kar dunga. 💪
