#ifndef DEBUG_UI_HPP
#define DEBUG_UI_HPP

struct GLFWwindow;
struct AppState;

bool initDebugUi(GLFWwindow* window);
void beginDebugUiFrame();
void drawDebugUi(AppState& state);
void endDebugUiFrame();
void shutdownDebugUi();

#endif
