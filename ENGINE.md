# OpenGL Horror Game Engine Reference

This document is a hands-on reference for the current engine codebase.
It is written to help you (or a new contributor) understand how the core
systems work, where the code lives, and what to change when extending it.

Scope and style
- This is a game-focused OpenGL 3.3 engine, not a general-purpose engine.
- Code style favors simple C++17 and direct data flow over heavy abstraction.
- AppState is the central data hub; most systems read from or write to it.
- Paths are relative to the project root unless noted otherwise.
- All file and symbol names listed here are current as of this document.

Build and run
- Build uses the custom nobuild system: `nob.cpp`.
- Build command: `./nob`.
- Build and run command: `./nob run`.
- Rebuild nob: `clang++ nob.cpp -o nob`.
- Dependencies: clang++, GLFW, GLAD, stb_image, miniaudio, Assimp.
- macOS build uses `/opt/homebrew` include and lib paths.

Repository layout (core files)
- `src/main.cpp`: GLFW setup, window creation, main loop.
- `src/app.hpp`: AppState definition and global engine settings.
- `src/app.cpp`: Engine frame loop, input routing, audio triggers, UI, render.
- `src/scene/camera.hpp`: Camera state, movement parameters, cooldowns.
- `src/scene/camera.cpp`: Camera movement, input handling, view matrix.
- `src/scene/world.hpp`: World-facing API for terrain, grass, trees, models.
- `src/scene/world.cpp`: Terrain, heightmap, grass, tree scatter, collisions.
- `src/render/renderer.hpp`: Renderer entry points.
- `src/render/renderer.cpp`: GL setup, buffers, draw pipeline, FBO scaling.
- `src/render/shader.hpp`: Shader wrapper interface.
- `src/render/shader.cpp`: Shader loading, compilation, linking, uniforms.
- `src/render/model.hpp`: Model types, asset/instance definitions.
- `src/render/model.cpp`: Assimp loader, mesh building, textures.
- `src/ui/debugUi.hpp`: ImGui entry points.
- `src/ui/debugUi.cpp`: Debug and map editor windows.
- `src/ui/mapEditor.hpp`: Editor picking and gizmo hooks.
- `src/ui/mapEditor.cpp`: Picking math and ImGuizmo manipulation.
- `Shader/default.vs`: Main world vertex shader.
- `Shader/default.fs`: Main world fragment shader (lighting, fog, alpha).
- `Shader/grass.vs`: Grass vertex shader (instancing).
- `Shader/grass.fs`: Grass fragment shader (alpha cutout, fog).
- `Shader/skybox.vs`: Skybox vertex shader.
- `Shader/skybox.fs`: Skybox fragment shader.
- `assets/`: Models, textures, skybox, audio, heightmap.
- `third_party/`: GLAD, stb_image, ImGui, ImGuizmo, miniaudio sources.

Boot sequence (startup order)
1) `main()` initializes GLFW and sets OpenGL 3.3 core profile.
2) A fullscreen window is created using the primary monitor.
3) Cursor is captured and swap interval is disabled (no vsync).
4) Window and framebuffer sizes are printed for debugging.
5) `AppInit()` is called and wires up callbacks and subsystems.
6) `renderInit()` sets GL state, loads shaders, creates buffers and textures.
7) `initWorldModels()` loads all models and populates initial instances.
8) `initDebugUi()` initializes ImGui backends and context.
9) `initAudio()` creates the miniaudio engine and loads all sounds.
10) Main loop begins and calls `AppFrame()` each iteration.

Frame update order (AppFrame)
- Delta time is computed from GLFW time and stored in AppState.
- `processInput()` handles editor toggle, free cam, wireframe, and UI toggle.
- Camera keyboard handling runs via `camera.ProcessKeyboard()`.
- Flashlight toggle sound plays when camera indicates it toggled.
- ImGui is updated if debug UI or editor is enabled.
- Editor selection uses ray picking and then draws ImGuizmo.
- `updateGroundCollision()` applies gravity and terrain collision.
- `updateFlashlightAttachment()` moves the flashlight model to the camera.
- Footstep audio is triggered based on grounded/movement/sprinting state.
- `renderFrame()` draws the scene into the render scale FBO.
- ImGui draw data is rendered to the backbuffer.
- GLFW swaps buffers and polls events.

Global state model (AppState)
- AppState is a single struct that owns most runtime state.
- It is passed to almost every subsystem, either by ref or const ref.
- It stores configuration values (render scale, lighting values, fog, etc).
- It stores runtime flags (free cam, editor enabled, dirty flags).
- It stores all live data: models, instances, buffers, textures.
- It also stores subsystem instances: Camera and AudioSystem.

Input map (core keys)
- W/A/S/D: move in the world (flat movement relative to camera forward).
- Mouse move: yaw and pitch changes the view direction.
- Q: toggle cursor grab (mouse look vs cursor mode).
- E: toggle editor mode (also toggles free cam and lighting overrides).
- V: toggle free cam (flying without gravity).
- C: crouch toggle or fast downward move in free cam/editor.
- Space: jump (grounded) or fly upward in free cam.
- F: toggle flashlight (spotlight + model visibility logic).
- G: toggle debug UI.
- P: toggle wireframe rendering.
- K: toggle fullscreen/windowed.
- 1/2/3: ImGuizmo mode (translate/rotate/scale).

Coordinate spaces used in the engine
- World space: all positions of terrain, models, grass, and camera.
- View space: camera space after view matrix multiplication.
- Clip space: result of projection * view * model.
- NDC: clip space divided by w (-1..1).
- Screen space: framebuffer or window coordinates.

Camera math summary
- `cameraFront` is built from yaw/pitch using standard spherical conversion.
- `flatFront` is `cameraFront` projected to the XZ plane (y = 0).
- Movement uses `flatFront` to avoid stopping when looking down.
- `cameraUp` is constant (0,1,0) and defines the world vertical axis.
- `GetViewMatrix()` uses `lookAt` with a bobbing offset in Y.

Movement and physics rules
- Gravity accumulates into `camera.velocity.y` when not grounded.
- Horizontal velocity is integrated in camera.ProcessKeyboard().
- Acceleration is applied when input exists; friction otherwise.
- Jump is allowed only when grounded and the cooldown is ready.
- Sprinting is allowed only if movement is mostly forward.
- Sneaking toggles camera height between 1.0 and 1.5.
- Free cam ignores gravity and uses high speed movement.

Terrain and heightmap
- Heightmap is 16-bit grayscale: `assets/heightmap_16bit.png`.
- `loadHeightmap()` uses `stbi_load_16` and caches data in world.cpp.
- `sampleHeightNormalized(u,v)` performs bilinear sampling on the heightmap.
- `getTerrainHeightAt(worldX, worldZ)` converts world position to UV.
- Terrain bounds are based on `floorSize` and `cubeScale`.
- Height = baseHeight + heightmapScale * heightNormalized.

Terrain mesh generation (buildFloor)
- Mesh is a continuous grid, not cubes.
- Grid resolution = (floorSize * 2) / terrainResolutionScale.
- Each vertex stores position, normal, and uv.
- Normals are computed via central differences on the height field.
- Indices are built as two triangles per grid cell.
- Terrain buffers live in VAO/VBO/EBO in AppState.

Grass system
- Grass is instanced quads arranged in an X shape.
- The quad size depends on `cubeScale`.
- Instances are spawned in a radius around the camera.
- `grassDensity` controls instances per tile.
- `grassRenderRadius` controls the spawn circle size.
- `grassDirty` triggers a rebuild of instance positions.
- Grass uses alpha cutout in `Shader/grass.fs`.

Tree system
- Trees are model instances placed by `scatterTrees()`.
- Placement uses a coarse cell grid and deterministic hashing.
- `treeDensity` drives how many instances per cell.
- A min-distance constraint avoids clustering.
- `treeRenderRadius` is applied in uploadTreeInstances() for visibility.
- Trees use instanced rendering and an instance matrix VBO.
- `treeInstanceDirty` triggers a new instance buffer upload.
- `treeDirty` rebuilds the scatter list (trees are re-generated).

Tree free-area zones
- Each ModelInstance has `freeArea` and `freeAreaRadius`.
- When freeArea is enabled, tree scatter skips that XZ radius.
- Radius of 0 means no exclusion (normal tree behavior).
- Tree free-area is currently used by the editor to avoid overlap.
- Rebuild is triggered by setting `state.treeDirty`.

Collision system
- Ground collision is resolved in `updateGroundCollision()`.
- Tree collision is a simple XZ radius push-out against `treeCollisionPositions`.
- Tree collision uses scaled radii based on `cubeScale`.
- No per-model collision for props yet.

Model system overview
- Models are loaded through Assimp into Model assets.
- Each Model contains a list of ModelMesh entries.
- Each ModelMesh stores VAO/VBO/EBO and texture ids.
- Each ModelAsset owns a Model and render settings.
- Each ModelInstance references an asset by index and holds transforms.

Model loading path (high level)
- `Model::Load()` resets previous GPU resources.
- Assimp flags: triangulate, calc tangent, gen normals, pre-transform.
- UV flipping is optional per model (settings.flipUv).
- Vertex layout: position(3), normal(3), uv(2), tangent(3) = 11 floats.
- Textures are resolved using material properties and fallback paths.
- Normal maps are optional and default to a flat normal texture.
- Each mesh ends with a VAO that is ready for draw calls.

Renderer pipeline overview
- Rendering is done to a low-res FBO, then upscaled.
- `renderScale` controls internal resolution and pixelated look.
- `ensureRenderTarget()` reallocates the FBO if size changes.
- The world shader (`Shader/default`) handles terrain and models.
- The grass shader handles grass cards with alpha cutout.
- The skybox shader draws a cubemap with adjustable intensity.

Lighting model
- Base lighting is directional (moon) + flashlight spotlight.
- Ambient, diffuse, specular factors are tunable in AppState.
- Shininess controls specular exponent.
- Normal maps are applied when enabled in ModelRenderSettings.
- Fog is exponential, applied in both world and grass shaders.

Flashlight system
- Flashlight is both a model and a spotlight.
- `updateFlashlightAttachment()` positions the model using camera basis.
- The flashlight spotlight direction can be overridden by model matrix.
- Flashlight intensity is zero when disabled, otherwise uses brightness.
- The flashlight model is hidden during editor mode if desired.

Skybox system
- Skybox uses cubemap textures from `assets/skybox/`.
- Shader multiplies sampled color by `skyboxIntensity`.
- View matrix translation is removed for skybox.
- Depth func is set to LEQUAL for skybox render pass.

Editor and picking
- Editor mode toggled by E key.
- Mouse must be ungrabbed with Q to interact with UI or gizmo.
- Picking uses ray cast from mouse to model bounding spheres.
- Selection is stored in AppState.selectedInstance.
- ImGuizmo transforms the selected instance in world space.
- Hotkeys 1/2/3 switch translate/rotate/scale.

UI layers
- Debug UI provides sliders for lighting, fog, terrain, grass, and trees.
- Map editor UI provides model spawning and free-area controls.
- ImGui is rendered after the 3D scene to the default framebuffer.

Audio system
- Audio uses miniaudio with a single engine.
- All sounds are loaded at init; errors are reported generically.
- `playSound` stops a sound before replaying to avoid overlap.
- `startLoopingSound` only starts if the sound is not already playing.
- `masterVolume` is applied to the engine globally.

Important data flows (dirty flags)
- Heightmap scale change -> terrainDirty -> rebuild terrain mesh.
- Terrain rebuild -> updateWorldModelHeights -> grassDirty and treeInstanceDirty.
- grassDirty -> buildGrass + uploadGrassInstances.
- treeDirty -> rebuildWorldTrees (re-scatters trees) + treeInstanceDirty.
- treeInstanceDirty -> uploadTreeInstances.

Common extension tasks
- Add a new model: update gModelTemplates in `src/render/model.cpp`.
- Spawn a model instance: call addModelInstance with asset index.
- Add a new sound: update AudioPaths and SoundId in audio.hpp.
- Add a new shader: create Shader/* files and load in renderInit().
- Add editor UI control: update `drawMapEditorUi`.
- Add a new uniform: add to shader and set in renderer before drawing.

Known limitations and tradeoffs
- Tree scatter uses CPU sampling; large densities can still be heavy.
- No physics engine; collisions are handcrafted.
- No scene serialization yet.
- No material system; ModelRenderSettings are per asset only.
- Lighting is forward and not physically based.
- Instancing is used only for trees and grass.
- Culling is coarse (treeRenderRadius and grassRenderRadius).

Glossary
- FBO: Framebuffer Object, the offscreen render target.
- VBO: Vertex Buffer Object, holds vertex data on GPU.
- VAO: Vertex Array Object, holds attribute layout.
- EBO: Element Buffer Object, holds indices.
- TBN: Tangent, Bitangent, Normal basis for normal mapping.
- NDC: Normalized Device Coordinates.
- Assimp: Asset importer used for FBX and other formats.

## Function Index (src)
### src/main.cpp
- Function main: see src/main.cpp for implementation details.

### src/app.cpp
- Function processInput: see src/app.cpp for implementation details.
- Function mouse_callback: see src/app.cpp for implementation details.
- Function framebuffer_size_callback: see src/app.cpp for implementation details.
- Function AppInit: see src/app.cpp for implementation details.
- Function AppFrame: see src/app.cpp for implementation details.
- Function AppShutdown: see src/app.cpp for implementation details.
- Function processInput: see src/app.cpp for implementation details.
- Function mouse_callback: see src/app.cpp for implementation details.
- Function framebuffer_size_callback: see src/app.cpp for implementation details.

### src/scene/camera.cpp

### src/scene/world.cpp
- Function clampFloat: see src/scene/world.cpp for implementation details.
- Function hashToUnitFloat: see src/scene/world.cpp for implementation details.
- Function addModelAsset: see src/scene/world.cpp for implementation details.
- Function addModelInstance: see src/scene/world.cpp for implementation details.
- Function findInstanceIndexByAsset: see src/scene/world.cpp for implementation details.
- Function scatterTrees: see src/scene/world.cpp for implementation details.
- Function spacingGrid: see src/scene/world.cpp for implementation details.
- Function resolveTreeCollisions: see src/scene/world.cpp for implementation details.
- Function loadHeightmap: see src/scene/world.cpp for implementation details.
- Function sampleHeightNormalized: see src/scene/world.cpp for implementation details.
- Function getTerrainHeightAt: see src/scene/world.cpp for implementation details.
- Function buildFloor: see src/scene/world.cpp for implementation details.
- Function buildGrass: see src/scene/world.cpp for implementation details.
- Function updateGroundCollision: see src/scene/world.cpp for implementation details.
- Function initWorldModels: see src/scene/world.cpp for implementation details.
- Function rebuildWorldTrees: see src/scene/world.cpp for implementation details.
- Function updateWorldModelHeights: see src/scene/world.cpp for implementation details.
- Function updateFlashlightAttachment: see src/scene/world.cpp for implementation details.

### src/audio/audio.cpp
- Function loadSound: see src/audio/audio.cpp for implementation details.
- Function initAudio: see src/audio/audio.cpp for implementation details.
- Function shutdownAudio: see src/audio/audio.cpp for implementation details.
- Function setMasterVolume: see src/audio/audio.cpp for implementation details.
- Function playSound: see src/audio/audio.cpp for implementation details.
- Function startLoopingSound: see src/audio/audio.cpp for implementation details.
- Function stopSound: see src/audio/audio.cpp for implementation details.

### src/render/renderer.cpp
- Function loadCubemap: see src/render/renderer.cpp for implementation details.
- Function uploadTerrainBuffers: see src/render/renderer.cpp for implementation details.
- Function uploadGrassInstances: see src/render/renderer.cpp for implementation details.
- Function uploadTreeInstances: see src/render/renderer.cpp for implementation details.
- Function ensureRenderTarget: see src/render/renderer.cpp for implementation details.
- Function buildCameraBasis: see src/render/renderer.cpp for implementation details.
- Function buildFlashlightModelMatrix: see src/render/renderer.cpp for implementation details.
- Function buildCameraBasis: see src/render/renderer.cpp for implementation details.
- Function buildFlashlightModelMatrix: see src/render/renderer.cpp for implementation details.
- Function renderInit: see src/render/renderer.cpp for implementation details.
- Function renderFrame: see src/render/renderer.cpp for implementation details.
- Function renderShutdown: see src/render/renderer.cpp for implementation details.
- Function loadCubemap: see src/render/renderer.cpp for implementation details.
- Function uploadTerrainBuffers: see src/render/renderer.cpp for implementation details.
- Function uploadGrassInstances: see src/render/renderer.cpp for implementation details.
- Function uploadTreeInstances: see src/render/renderer.cpp for implementation details.
- Function ensureRenderTarget: see src/render/renderer.cpp for implementation details.

### src/render/shader.cpp

### src/render/model.cpp
- Function makeTreeSettings: see src/render/model.cpp for implementation details.
- Function makeFlashlightSettings: see src/render/model.cpp for implementation details.
- Function makeChurchSettings: see src/render/model.cpp for implementation details.
- Function makeWalterSettings: see src/render/model.cpp for implementation details.
- Function makeDeadTreeSettings: see src/render/model.cpp for implementation details.
- Function GetModelTemplates: see src/render/model.cpp for implementation details.
- Function getDirectory: see src/render/model.cpp for implementation details.
- Function getFileName: see src/render/model.cpp for implementation details.
- Function joinPath: see src/render/model.cpp for implementation details.
- Function toLower: see src/render/model.cpp for implementation details.
- Function chooseFallbackBaseName: see src/render/model.cpp for implementation details.
- Function chooseUvChannel: see src/render/model.cpp for implementation details.
- Function getDefaultWhiteTexture: see src/render/model.cpp for implementation details.
- Function getDefaultNormalTexture: see src/render/model.cpp for implementation details.
- Function loadTextureFromFile: see src/render/model.cpp for implementation details.
- Function loadTextureByBaseName: see src/render/model.cpp for implementation details.
- Function loadMaterialTexture: see src/render/model.cpp for implementation details.
- Function loadDiffuseTexture: see src/render/model.cpp for implementation details.
- Function loadNormalTexture: see src/render/model.cpp for implementation details.
- Function getDefaultNormalTexture: see src/render/model.cpp for implementation details.

### src/ui/debugUi.cpp
- Function initDebugUi: see src/ui/debugUi.cpp for implementation details.
- Function beginUiFrame: see src/ui/debugUi.cpp for implementation details.
- Function drawDebugUi: see src/ui/debugUi.cpp for implementation details.
- Function drawMapEditorUi: see src/ui/debugUi.cpp for implementation details.
- Function endDebugUiFrame: see src/ui/debugUi.cpp for implementation details.
- Function shutdownDebugUi: see src/ui/debugUi.cpp for implementation details.

### src/ui/mapEditor.cpp
- Function getMouseRay: see src/ui/mapEditor.cpp for implementation details.
- Function pickModelInstance: see src/ui/mapEditor.cpp for implementation details.
- Function buildInstanceMatrix: see src/ui/mapEditor.cpp for implementation details.
- Function getGizmoOperation: see src/ui/mapEditor.cpp for implementation details.
- Function updateGizmoHotkeys: see src/ui/mapEditor.cpp for implementation details.
- Function handleEditorPicking: see src/ui/mapEditor.cpp for implementation details.
- Function rayOrigin: see src/ui/mapEditor.cpp for implementation details.
- Function rayDir: see src/ui/mapEditor.cpp for implementation details.
- Function updateMapEditorGizmo: see src/ui/mapEditor.cpp for implementation details.
- Function getMouseRay: see src/ui/mapEditor.cpp for implementation details.
- Function pickModelInstance: see src/ui/mapEditor.cpp for implementation details.
- Function buildInstanceMatrix: see src/ui/mapEditor.cpp for implementation details.
- Function getGizmoOperation: see src/ui/mapEditor.cpp for implementation details.
- Function updateGizmoHotkeys: see src/ui/mapEditor.cpp for implementation details.

## AppState Field Index
This is a raw index of fields defined in AppState (src/app.hpp).
Each line lists the field and a short hint for where it is used.
- AppState.cubeScale (float): World unit scale that affects terrain size, grass size, and collisions.
- AppState.deltaTime (float): Runtime state or resource.
- AppState.lastFrame (float): Runtime state or resource.
- AppState.floorSize (int): Runtime state or resource.
- AppState.floorY (float): Runtime state or resource.
- AppState.terrainResolutionScale (float): Runtime state or resource.
- AppState.renderDistance (float): Far plane distance used in projection and culling.
- AppState.ambientStrength (float): Runtime state or resource.
- AppState.diffuseStrength (float): Runtime state or resource.
- AppState.specularStrength (float): Runtime state or resource.
- AppState.shininess (float): Runtime state or resource.
- AppState.moonDir (glm::vec3): Runtime state or resource.
- AppState.moonColor (glm::vec3): Runtime state or resource.
- AppState.flashlightBrightness (float): Runtime state or resource.
- AppState.flashlightRadius (float): Runtime state or resource.
- AppState.flashlightColor (glm::vec3): Runtime state or resource.
- AppState.flashlightOffsetForward (float): Runtime state or resource.
- AppState.flashlightOffsetRight (float): Runtime state or resource.
- AppState.flashlightOffsetDown (float): Runtime state or resource.
- AppState.flashlightBeamOffset (glm::vec3): Runtime state or resource.
- AppState.flashlightBeamForward (glm::vec3): Runtime state or resource.
- AppState.flashlightShown (bool): Runtime state or resource.
- AppState.fogDensity (float): Runtime state or resource.
- AppState.fogColor (glm::vec3): Runtime state or resource.
- AppState.heightmapScale (float): Multiplier for heightmap amplitude.
- AppState.terrainDirty (bool): Runtime state or resource.
- AppState.grassDensity (float): Grass instances per tile around camera.
- AppState.grassIntensity (float): Runtime state or resource.
- AppState.grassRenderRadius (float): Runtime state or resource.
- AppState.grassDirty (bool): Runtime state or resource.
- AppState.grassCenter (glm::vec2): Runtime state or resource.
- AppState.treeDensity (float): Scatter density used in tree placement.
- AppState.treeDirty (bool): Runtime state or resource.
- AppState.treeInstanceDirty (bool): Runtime state or resource.
- AppState.treeFreeAreaRadius (float): Default radius for free-area exclusion zones.
- AppState.treeRenderRadius (float): Visibility radius for tree instancing.
- AppState.treeUpdateDistance (float): Runtime state or resource.
- AppState.treeCullCenter (glm::vec2): Runtime state or resource.
- AppState.skyboxIntensity (float): Runtime state or resource.
- AppState.editorEnabled (bool): Runtime state or resource.
- AppState.selectedInstance (int): Runtime state or resource.
- AppState.gizmoOperation (int): Runtime state or resource.
- AppState.editorWantsMouse (bool): Runtime state or resource.
- AppState.editorHasSavedValues (bool): Runtime state or resource.
- AppState.editorSavedRenderDistance (float): Runtime state or resource.
- AppState.editorSavedAmbientStrength (float): Runtime state or resource.
- AppState.editorSavedSkyboxIntensity (float): Runtime state or resource.
- AppState.editorSavedFogDensity (float): Runtime state or resource.
- AppState.fullscreen (bool): Runtime state or resource.
- AppState.wireframe (bool): Runtime state or resource.
- AppState.freeCam (bool): Runtime state or resource.
- AppState.showDebugUi (bool): Runtime state or resource.
- AppState.camera (Camera): Camera object with input and view data.
- AppState.audio (AudioSystem): AudioSystem (miniaudio engine and sounds).
- AppState.worldShader (Shader*): Main world shader used for terrain and models.
- AppState.skyboxShader (Shader*): Shader used for the cubemap skybox.
- AppState.grassShader (Shader*): Shader used for grass cards.
- AppState.modelAssets (std::vector<ModelAsset>): Loaded model assets (meshes and render settings).
- AppState.modelInstances (std::vector<ModelInstance>): Placed model instances with transforms.
- AppState.treeAssetIndex (int): Runtime state or resource.
- AppState.treeInstanceIndex (int): Runtime state or resource.
- AppState.walterAssetIndex (int): Runtime state or resource.
- AppState.walterInstanceIndex (int): Runtime state or resource.
- AppState.flashlightAssetIndex (int): Runtime state or resource.
- AppState.flashlightInstanceIndex (int): Runtime state or resource.
- AppState.churchAssetIndex (int): Runtime state or resource.
- AppState.churchInstanceIndex (int): Runtime state or resource.
- AppState.deadtreeAssetIndex (int): Runtime state or resource.
- AppState.deadtreeInstanceIndex (int): Runtime state or resource.
- AppState.walterScale (float): Runtime state or resource.
- AppState.churchScale (float): Runtime state or resource.
- AppState.deadtreeScale (float): Runtime state or resource.
- AppState.flashlightScale (float): Runtime state or resource.
- AppState.VBO (unsigned int): Runtime state or resource.
- AppState.VAO (unsigned int): Runtime state or resource.
- AppState.EBO (unsigned int): Runtime state or resource.
- AppState.skyboxVAO (unsigned int): Runtime state or resource.
- AppState.skyboxVBO (unsigned int): Runtime state or resource.
- AppState.cubemapTexture (unsigned int): Runtime state or resource.
- AppState.texture (unsigned int): Runtime state or resource.
- AppState.grassVAO (unsigned int): Runtime state or resource.
- AppState.grassVBO (unsigned int): Runtime state or resource.
- AppState.grassEBO (unsigned int): Runtime state or resource.
- AppState.grassInstanceVBO (unsigned int): Runtime state or resource.
- AppState.grassTexture (unsigned int): Runtime state or resource.
- AppState.grassIndexCount (int): Runtime state or resource.
- AppState.treeInstanceVBO (unsigned int): Runtime state or resource.
- AppState.treeInstanceCount (int): Runtime state or resource.
- AppState.treeCollisionPositions (std::vector<glm::vec3>): Runtime state or resource.
- AppState.renderTargetFbo (unsigned int): Runtime state or resource.
- AppState.renderTargetColor (unsigned int): Runtime state or resource.
- AppState.renderTargetDepth (unsigned int): Runtime state or resource.
- AppState.renderTargetWidth (int): Runtime state or resource.
- AppState.renderTargetHeight (int): Runtime state or resource.
- AppState.terrainVertices (std::vector<float>): Runtime state or resource.
- AppState.terrainIndices (std::vector<unsigned int>): Runtime state or resource.
- AppState.grassInstances (std::vector<glm::vec3>): Runtime state or resource.

## Shader Uniform Index
### Shader/default.vs
- uniform mat4 model
- uniform mat4 view
- uniform mat4 projection
- uniform bool useInstancing

### Shader/default.fs
- uniform sampler2D ourTexture
- uniform sampler2D normalMap
- uniform bool useNormalMap
- uniform float normalStrength
- uniform bool normalDebug
- uniform bool depthOnly
- uniform bool doubleSided
- uniform vec3 lightDir
- uniform vec3 lightColor
- uniform vec3 viewPos
- uniform float ambientStrength
- uniform float diffuseStrength
- uniform float specularStrength
- uniform float shininess
- uniform float alphaCutoff
- uniform vec3 spotPos
- uniform vec3 spotDir
- uniform vec3 spotColor
- uniform float spotIntensity
- uniform float spotInnerCutoff
- uniform float spotOuterCutoff
- uniform vec3 fogColor
- uniform float fogDensity
- uniform float albedoIntensity

### Shader/grass.vs
- uniform mat4 view
- uniform mat4 projection

### Shader/grass.fs
- uniform sampler2D grassTexture
- uniform vec3 lightDir
- uniform vec3 lightColor
- uniform vec3 viewPos
- uniform float ambientStrength
- uniform float diffuseStrength
- uniform vec3 spotPos
- uniform vec3 spotDir
- uniform vec3 spotColor
- uniform float spotIntensity
- uniform float spotInnerCutoff
- uniform float spotOuterCutoff
- uniform float grassIntensity
- uniform vec3 fogColor
- uniform float fogDensity

### Shader/skybox.vs
- uniform mat4 projection
- uniform mat4 view

### Shader/skybox.fs
- uniform samplerCube skybox
- uniform float skyboxIntensity

## Model Template Index
- Template {"Tree", "assets/models/pine_tree/source/pine_tree.fbx"
- Template {"WalterWhite", "assets/models/walter_white/source/Hussainberg.fbx"
- Template {"Church", "assets/models/church/source/church.fbx", makeChurchSettings()
- Template {"Flashlight", "assets/models/flashlight/source/Flashlight.fbx"
- Template {"Dead_Tree", "assets/models/dead_tree/source/dead_tree.fbx"
