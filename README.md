<p align="center">
  <img src="img.jpg" width="80" alt="madoka"><br/>
  <b>Madokawaii</b>
</p>


<p align="center">
  山有木兮木有枝，心悦君兮君不知。
</p>

![GitHub top language](https://img.shields.io/github/languages/top/lucas150670/Madokawaii?style=for-the-badge)
![GitHub commit activity](https://img.shields.io/github/commit-activity/w/lucas150670/Madokawaii?style=for-the-badge)
![Telegram](https://telegram-badge.vercel.app/api/telegram-badge?channelId=@MadokawaiiChat&style=for-the-badge)
![GitHub License](https://img.shields.io/github/license/lucas150670/Madokawaii?style=for-the-badge)
[![State-of-the-art Shitcode](https://img.shields.io/static/v1?label=State-of-the-art&message=Shitcode&color=7B5804&style=for-the-badge)](https://github.com/trekhleb/state-of-the-art-shitcode)

Madokawaii is an early-stage rhythm game simulator inspired by *Phigros*, aiming to it's clean-room reimplementation.

The project currently focuses on implementing autoplay functionality for official Phigros chart format (version 3) and PhiEdit format.

Future development will expand toward interactive gameplay, cross-platform support, and advanced rendering features.

NOTICE: raylib is the **only reference implementation** of the render backend.
this project makes no guarantees regarding other backends' operational results.

---

## 🛠 INSTALLATION & RUNNING

### Prerequisite
- cmake
- vcpkg
- raylib, rapidjson, libyaml, libzip, libzippp, fastio(2024-12-05, MIT) (managed by vcpkg manifest mode)
- Windows SDK & DirectX SDK (when Direct2D backend enabled)
- FMOD Core SDK (when FMOD audio backend enabled)
- CRI ADX LE Native SDK, stb-vorbis, drlibs (when CRI audio backend enabled)
- for Android project, see [Madokawaii_Android](https://github.com/lucas150670/Madokawaii_Android) 

## 📌 EXAMPLE
```bash
# Install vcpkg and dependencies
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg integrate install
# Build with cmake
mkdir build && cd build
# switch from render backends, audio backends by defining implementer variable
# notice: Platform projects, like Android, are built using their own project manager.
# currently, CMake bootstrap compilation is only available on Windows and Linux platforms.
cmake .. -Dimplementer=RAYLIB -Daudio_implementer=RAYLIB -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
make
# put assets/charts/chart.json,
# assets/charts/music.ogg,
# assets/charts/illustration.png,
# assets/respack/default.zip 
# in correct position.
./Madokawaii
```
---

## 📂 STRUCTURE
```
Madokawaii/
├── include/            # header file
    └── Madokawaii/
        ├── app/        # app logic
        └── platform/   # platform interfaces
├── src/                # source
    ├── app             # app logic
    └── platform        # platfor-specific code
       ├── raylib       # reference implementaion based on raylib
       ├── direct2d     # Direct2D Windows backend
       ├── d3d11on12    # experimental DirectX 12 support, based on D3D11on12 and Direct2D backend.
       ├── gdiplus      # i can't think of any reason to enable this component unless you have CP.
       ├── fmod         # experimental FMOD audio backend
       ├── cri          # experimental CRI ADX LE audio backend
       └── (other potential platforms)
├── README.md           # readme
└── CMakeLists.txt      # cmake config
```

Notice: 
- the gdiplus backend is solely rendered on CPU and has a very bad performance, approximately 1/30 of Direct2D and 1/60 of raylib, and may have lots of bugs.
- both FMOD and CRI ADX LE are proprietary sdks. their corresponding files are excluded from version control via `.gitigore`.
- to access FMOD audio backend, you need to have a valid FMOD license, and access to FMOD Core SDK. put the fmod dependencies in src/platform/fmod/inc, src/platform/fmod/lib/$ARCH, see [CMakeLists.txt](CMakeLists.txt).
- to access CRI ADX LE backend, you need to comply with [LICENSE AGREEMENT FOR CRI ADX LE](https://game.criware.jp/products/adx-le/terms/), then download the ADX LE Native SDK on [CRI ADX LE](https://game.criware.jp/products/adx-le/). put the whole SDK under src/platform/cri.
- both FMOD and CRI ADX LE requires attribution screen before startup. to comply with their license agreement, download their logo on their official website, then put it in the corresponding directory in [CMakeLists.txt](CMakeLists.txt). the build system will copy them to the designated asset folder.

---

## 🤝 CONTRIBUTION
Contributions are welcome from all individuals.

---

## 🔜 ROADMAP
1. Phigros Official Format Version 1,2 / Re:Phigros Edit chart format, parse & support
2. implement HarmonyOS backend (ArkTS-side XComponent wrap, interaction with native module) (maybe 2026)
3. libuv / asynchronous fileio, chart load & parse, draw queue (maybe 2026)
4. rpe storyboard / shader support (maybe 2026/2027)
5. user playable (2027)

---

## 📄 LICENSE
Copyright 2023-2026 Madokawaii

Licensed under [the Apache License, Version 2.0](LICENSE) (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

This project contains visual, narrative, or thematic elements 
inspired by *Puella Magi Madoka Magica*. All original characters, artwork, storylines, 
and intellectual property related to *Madoka Magica* are © Magica Quartet / Aniplex, 
Madoka Partners, MBS. 

---

## 📬 CONTACT
- maintainer: lucas150670
- mail: lucas150670@petalmail.com
- telegram group: [@MadokawaiiChat](https://t.me/MadokawaiiChat)