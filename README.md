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

Madokawaii is an early-stage rhythm game simulator inspired by *Phigros*. 

The project currently focuses on implementing autoplay functionality for official Phigros chart formats, version 3 and PhiEdit format.

Future development will expand toward interactive gameplay, cross-platform support, and advanced rendering features.

NOTICE: raylib is the **only reference implementation** of the render backend.
this project makes no guarantees regarding other backends' operational results.

---

## 🛠 INSTALLATION & RUNNING

### Prerequisite
- cmake
- vcpkg
- raylib, rapidjson, libyaml, libzip, libzippp, fastio(2024-12-05, MIT) (managed by vcpkg)
- Windows SDK & DirectX SDK (when Direct2D backend enabled)
- for Android project, see [here](https://github.com/lucas150670/Madokawaii_Android) 

## 📌 EXAMPLE
```bash
# Install vcpkg and dependencies
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install raylib rapidjson libyaml libzip libzippp 
# Build with cmake
mkdir build && cd build
# switch from render backends by defining implementer variable
cmake .. -Dimplementer=RAYLIB -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
make
# put assets/charts/chart.json,
# assets/charts/music.wav,
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
       ├── gdiplus      # i can't think of any reason to enable this component unless you have CP.
       └── (other potential platforms)
├── README.md           # readme
└── CMakeLists.txt      # cmake config
```

Notice: the gdiplus backend is solely rendered on CPU and has a very bad performance, approximately 1/30 of Direct2D and 1/60 of raylib, and may have lots of bugs.

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