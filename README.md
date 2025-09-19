<p align="center">
  <img src="img.jpg" width="100" alt="madoka">
</p>

<h1 align="center">
  Madokawaii
</h1>

<p align="center">
  山有木兮木有枝，心悦君兮君不知。
</p>

Madokawaii is an early-stage rhythm game simulator inspired by *Phigros*. 

The project currently focuses on implementing autoplay functionality for official Phigros chart formats.

Future development will expand toward interactive gameplay, cross-platform support, and advanced rendering features.

---

## 🛠 INSTALLATION & RUNNING

### Prerequisite
- cmake
- vcpkg
- raylib, rapidjson, libyaml (managed by vcpkg)


## 📌 EXAMPLE
```bash
# Install vcpkg and dependencies
git clone https://github.com/microsoft/vcpkg.git
./vcpkg/bootstrap-vcpkg.sh
./vcpkg/vcpkg install raylib rapidjson libyaml
# Build with cmake
mkdir build && cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
make
# put assets/charts/chart.json and assets/charts/music.wav in correct position.
./Madokawaii
```
---

## 📂 STRUCTURE
```
Madokawaii/
├── src/               # source
├── include/           # header file
├── README.md          # readme
└── CMakeLists.txt     # cmake config
```

---

## 🤝 CONTRIBUTION
Contributions are welcome from all individuals.

---

## 🔜 ROADMAP
1. refractor (2025)
2. HarmonyOS Support (Vulkan API support / ArkTS-side XComponent wrap, interaction with native module) (maybe 2026)
3. libuv / asynchronous fileio, chart load & parse, draw queue (maybe 2026)
4. rpe storyboard / shader support (maybe 2026/2027)

---

## 📄 LICENSE
Copyright 2023-2025 Madokawaii

Licensed under [the Apache License, Version 2.0](LICENSE) (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

---

## 📬 CONTACT
- maintainer: lucas150670
- mail：lucas150670@petalmail.com