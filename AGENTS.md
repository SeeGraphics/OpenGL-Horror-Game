# Agent Context: Project Horror Game in OpenGL 

## Core Rules
1. **No CMake/Make:** We use a custom C++ build system called `nop.cpp`.
2. **Handmade Style:** Prefer simple C++17. Avoid heavy OOP or "modern" over-abstractions.
3. **Environment:** macOS Silicon. Libraries are in `/opt/homebrew/`.

## Tech Stack
- **Language:** C++17
- **Graphics API:** OpenGL 3.3 (Core Profile)
- **Windowing:** GLFW
- **Math:** GLM
- **UI:** ImGui (Dark Mode)
- **Loader:** GLAD / stb_image

## Project Structure
- `src/`: Core logic (`main.cpp`, `camera.cpp`, `shader.cpp`)
- `include/`: Headers and `primitives.hpp`
- `assets/`: Textures and Skyboxes
- `Shader/`: `.vs` and `.fs` files
- `nob.cpp`: The build system (executable is `./nop`)

## Current Focus (read todo.txt for more information on next steps i planned)
We are currently working on a Horror Atmosphere. This includes:
- Dim ambient lighting.
- Flashlight logic (Spotlight) 
- Normal mapping for cubes.

## Tests
compile with: ./nop
run with: ./nop run

always check if it compiles first.