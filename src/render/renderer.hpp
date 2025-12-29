#ifndef RENDERER_HPP
#define RENDERER_HPP

struct GLFWwindow;
struct AppState;

bool renderInit(AppState& state, GLFWwindow* window);
void renderFrame(AppState& state, GLFWwindow* window);
void renderShutdown(AppState& state);

#endif
