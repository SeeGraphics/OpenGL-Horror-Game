#ifndef DEBUG_UI_HPP
#define DEBUG_UI_HPP

struct GLFWwindow;
struct AppState;

bool initDebugUi(GLFWwindow *window);
void beginUiFrame();
void beginMapEditorUiFrame();
void drawDebugUi(AppState &state);
void drawMapEditorUi(AppState &state);
void endDebugUiFrame();
void shutdownDebugUi();

#endif
