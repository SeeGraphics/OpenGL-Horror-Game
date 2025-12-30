#include <dirent.h>
#include <sys/stat.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

// Check if a file exists and get its last modified time
time_t get_mtime(const std::string& path) {
  struct stat s;
  if (stat(path.c_str(), &s) == 0) return s.st_mtime;
  return 0;
}

static time_t gHeaderTime = 0;

static bool isDotEntry(const char* name) {
  return std::strcmp(name, ".") == 0 || std::strcmp(name, "..") == 0;
}

static bool isHeaderFile(const char* name) {
  const char* dot = std::strrchr(name, '.');
  if (!dot) return false;
  return std::strcmp(dot, ".h") == 0 || std::strcmp(dot, ".hpp") == 0;
}

time_t getLatestHeaderTime(const std::string& root) {
  DIR* dir = opendir(root.c_str());
  if (!dir) return 0;

  time_t latest = 0;
  dirent* entry = nullptr;
  while ((entry = readdir(dir)) != nullptr) {
    if (isDotEntry(entry->d_name)) continue;
    std::string path = root + "/" + entry->d_name;
    struct stat s;
    if (stat(path.c_str(), &s) != 0) continue;

    if (S_ISDIR(s.st_mode)) {
      time_t childLatest = getLatestHeaderTime(path);
      if (childLatest > latest) latest = childLatest;
    } else if (S_ISREG(s.st_mode)) {
      if (isHeaderFile(entry->d_name)) {
        if (s.st_mtime > latest) latest = s.st_mtime;
      }
    }
  }

  closedir(dir);
  return latest;
}

bool needs_rebuild(const std::string& src, const std::string& obj) {
  time_t srcTime = get_mtime(src);
  time_t objTime = get_mtime(obj);
  if (objTime == 0) return true;  // Object doesn't exist
  if (srcTime > objTime) return true;
  return gHeaderTime > objTime;
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
      "-framework CoreFoundation -framework CoreAudio -framework AudioToolbox "
      "-lassimp";
  std::string flags = "-std=c++17 -Wall -Wextra";

  time_t srcHeaderTime = getLatestHeaderTime("src");
  time_t thirdPartyHeaderTime = getLatestHeaderTime("third_party");
  gHeaderTime = (srcHeaderTime > thirdPartyHeaderTime) ? srcHeaderTime
                                                       : thirdPartyHeaderTime;

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
      {"src/render/model.cpp", "build/model.o"},
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
