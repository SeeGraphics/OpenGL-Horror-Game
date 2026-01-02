#ifndef MAP_EDITOR_HPP
#define MAP_EDITOR_HPP

struct AppState;
struct GLFWwindow;

enum GizmoMode {
  GizmoTranslate = 0,
  GizmoRotate = 1,
  GizmoScale = 2,
};

void handleEditorPicking(AppState& state, GLFWwindow* window);
void updateMapEditorGizmo(AppState& state, GLFWwindow* window);

#endif
