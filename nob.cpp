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
      "-I./include -I./imgui -I./imgui/backends -I/opt/homebrew/include";
  std::string lib =
      "-L/opt/homebrew/lib -lglfw -framework OpenGL -framework Cocoa "
      "-framework IOKit -framework CoreVideo";
  std::string flags = "-std=c++17 -Wall -Wextra";

  run_cmd("mkdir -p build");

  // GLAD (C)
  if (needs_rebuild("src/glad.c", "build/glad.o")) {
    run_cmd(
        cc +
        " -c src/glad.c -o build/glad.o -I./include -I/opt/homebrew/include");
  }

  // ImGui & Project Files (C++)
  std::vector<std::pair<std::string, std::string>> files = {
      {"imgui/imgui.cpp", "build/imgui.o"},
      {"imgui/imgui_draw.cpp", "build/imgui_draw.o"},
      {"imgui/imgui_widgets.cpp", "build/imgui_widgets.o"},
      {"imgui/imgui_tables.cpp", "build/imgui_tables.o"},
      {"imgui/backends/imgui_impl_glfw.cpp", "build/imgui_impl_glfw.o"},
      {"imgui/backends/imgui_impl_opengl3.cpp", "build/imgui_impl_opengl3.o"},
      {"src/main.cpp", "build/main.o"},
      {"src/camera.cpp", "build/camera.o"},
      {"src/shader.cpp", "build/shader.o"},
      {"src/stb_image.cpp", "build/stb_image.o"}};

  std::string all_objs = "build/glad.o ";
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
