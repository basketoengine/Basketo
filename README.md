# Basketo Game Engine


Welcome to the Basketo Engine - a passion project where I’m putting my best shot into creating something awesome. Whether you’re here to contribute, suggest ideas, or just watch it grow, you’re part of the journey!

## Discord: https://discord.gg/sTM6FPMH (NEW)

## Click the below image to see Video
[![Basketo Engine Demo](readmeimgs/image2.png)](https://x.com/BaslaelWorkneh/status/1922713614697288096)



## Building the Engine (Linux)

### Prerequisites
- CMake 3.26.0 or higher is required.
- SDL2, SDL2_image, SDL2_ttf, SDL2_mixer
- Lua 5.4
- g++ (C++17)

### Build Steps
```bash
# Clone the repository
git clone git@github.com:basketoengine/Basketo.git

cd Basketo

mkdir build && cd build

cmake ..

make -j$(nproc)

```

### Running
```bash
./BasketoGameEngine

```

If you want to run the physics test:
```bash
./PhysicsTest

```

## Building the Engine (Windows)

### Prerequisites

1. **Visual Studio 2022 (Community Edition or higher)**  
   Install with the following workloads:
   - Desktop development with C++
   - C++ CMake tools for Windows
   - Windows 10 or 11 SDK

2. **CMake** (included with Visual Studio or download from https://cmake.org/download/)

3. **vcpkg** (used to install dependencies)


---

### Install Dependencies

```bash
# Open a Developer Command Prompt for VS 2022 or PowerShell

# Clone vcpkg

git clone https://github.com/microsoft/vcpkg.git
cd vcpkg

.\bootstrap-vcpkg.bat

# Install required libraries
.\vcpkg install sdl2 sdl2-image sdl2-ttf sdl2-mixer lua

```

---

### Build Steps

```bash
# Clone the Basketo repository
git clone https://github.com/basketoengine/Basketo.git
cd Basketo
mkdir build && cd build

# Configure with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=C:/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release

# Replace "C:/path/to/vcpkg" with the full path to your vcpkg directory

# Build the engine
cmake --build . --config Release
```

---

### Running

```bash
# From the build/Release directory
./BasketoGameEngine.exe

# To run the physics test
./PhysicsTest.exe
```

## Get Involved

- Found a bug? Open an issue!  
- Have a cool feature idea? Let’s hear it!  
- Want to contribute? Fork, code, and create a pull request!  

Let’s build this engine together and make game dev fun and easy for everyone.

⭐ If you like the project, don’t forget to **star** and spread the word!

---

Happy coding and game making!


