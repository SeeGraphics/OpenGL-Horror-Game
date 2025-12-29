#include <sys/stat.h>

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

// Check if a file exists and get its last modified time
time_t get_mtime(const std::string& path) {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) return s.st_mtime;
  return 0;
}

bool needs_rebuild(const std::string& src, const std::string& obj) {
  time_t srcTime = get_mtime(src);
  time_t objTime = get_mtime(obj);
  if (objTime == 0) return true;  // Object doesn't exist
  return srcTime > objTime;       // Source is newer than object
}

void run_cmd(const std::string& cmd) {
  std::cout << "[CMD] " << cmd << std::endl;
  if (std::system(cmd.c_str()) != 0) exit(1);
}

int main(int argc, char** argv) {
  std::string cxx = "clang++", cc = "clang";
  std::string inc =
      "-I./src -I./third_party/imgui -I./third_party/imgui/backends "
      "-I./third_party/glad/include -I./third_party/stb "
      "-I./third_party/miniaudio "
      "-I/opt/homebrew/include";
  std::string lib =
      "-L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa "
      "-framework IOKit -framework CoreVideo "
      "-framework CoreFoundation -framework CoreAudio -framework AudioToolbox";
  std::string flags = "-std=c++17 -Wall -Wextra";

  run_cmd("mkdir -p build");

  // GLAD (C)
  if (needs_rebuild("third_party/glad/src/glad.c", "build/glad.o")) {
    run_cmd(cc + " -c third_party/glad/src/glad.c -o build/glad.o " + inc);
  }

  // miniaudio (C)
  if (needs_rebuild("third_party/miniaudio/miniaudio.c", "build/miniaudio.o")) {
    run_cmd(cc + " -c third_party/miniaudio/miniaudio.c -o build/miniaudio.o " +
            inc);
  }

  // ImGui & Project Files (C++)
  std::vector<std::pair<std::string, std::string>> files = {
      {"third_party/imgui/imgui.cpp", "build/imgui.o"},
      {"third_party/imgui/imgui_draw.cpp", "build/imgui_draw.o"},
      {"third_party/imgui/imgui_widgets.cpp", "build/imgui_widgets.o"},
      {"third_party/imgui/imgui_tables.cpp", "build/imgui_tables.o"},
      {"third_party/imgui/backends/imgui_impl_glfw.cpp",
       "build/imgui_impl_glfw.o"},
      {"third_party/imgui/backends/imgui_impl_opengl3.cpp",
       "build/imgui_impl_opengl3.o"},
      {"src/app.cpp", "build/app.o"},
      {"src/audio/audio.cpp", "build/audio.o"},
      {"src/ui/debugUi.cpp", "build/debugUi.o"},
      {"src/render/renderer.cpp", "build/renderer.o"},
      {"src/scene/world.cpp", "build/world.o"},
      {"src/main.cpp", "build/main.o"},
      {"src/scene/camera.cpp", "build/camera.o"},
      {"src/render/shader.cpp", "build/shader.o"},
      {"third_party/stb/stb_image.cpp", "build/stb_image.o"}};

  std::string all_objs = "build/glad.o build/miniaudio.o ";
  for (const auto& file : files) {
    if (needs_rebuild(file.first, file.second)) {
      run_cmd(cxx + " " + flags + " -c " + file.first + " -o " + file.second +
              " " + inc);
    }
    all_objs += file.second + " ";
  }

  // Linking (Always run this or check if any .o is newer than the binary)
  run_cmd(cxx + " " + all_objs + "-o build/game " + lib);

  if (argc > 1 && std::string(argv[1]) == "run") {
    run_cmd("./build/game");
  }

  return 0;
}
