# --- COMPILER & PATHS ---
CXX      = clang++
CC       = clang
BREW_INC = /opt/homebrew/include
BREW_LIB = /opt/homebrew/lib

# --- DIRECTORIES ---
BUILD_DIR = build
SRC_DIR   = src

# --- FLAGS ---
CXXFLAGS = -std=c++17 -Wall -Wextra -I./include -I$(BREW_INC)
CFLAGS   = -Wall -I./include -I$(BREW_INC)
LDFLAGS  = -L$(BREW_LIB) -lglfw -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo

# --- FILES ---
TARGET   = $(BUILD_DIR)/opengl_cube
SRC_CPP  = src/main.cpp src/camera.cpp src/shader.cpp src/stb_image.cpp
SRC_C    = src/glad.c

# Transform src/*.cpp and src/*.c into build/*.o
OBJ = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRC_CPP))
OBJ += $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_C))

# --- RULES ---
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJ)
	$(CXX) $(OBJ) -o $(TARGET) $(LDFLAGS)

# Rule for C++ files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Rule for C files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create the build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

run: all
	./$(TARGET)
