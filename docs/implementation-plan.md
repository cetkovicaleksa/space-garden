# Space Garden — Implementation Plan

Goal: a 2D and a 3D OpenGL (3.3 core, programmable pipeline only) app showing a small space-station garden: three pots, a shared lamp, and a water tank. Both apps share **one logic model**; each app only renders it and maps input to it.

Target: Ubuntu, C++17, CMake ≥ 3.20. Style follows LearnOpenGL (learnopengl.com) throughout.

Companion doc: `cg-notes.md` (computer graphics concepts reference). Each step lists which notes sections it touches.

---

## Architecture

```
space-garden/
├── CMakeLists.txt              # top level, FetchContent deps, add_subdirectory(...)
├── include/                    # header-only / vendored (committed to repo)
│   ├── glad/glad.h  KHR/khrplatform.h
│   ├── stb_image.h
│   └── learnopengl/            # shader.h, camera.h, mesh.h, model.h (adapted)
├── third_party/glad/glad.c     # the only vendored .c file (GLAD loader)
├── core/                       # garden_core: PURE C++, no OpenGL, no GLFW
│   ├── include/garden/Garden.h  Pot.h  Lamp.h  Tank.h  Droplet.h  Config.h  Command.h
│   └── src/...
├── engine/                     # garden_engine: shared GL helpers used by both apps
│   ├── Window (fullscreen, Esc, 75 FPS limiter)
│   ├── Texture loading, TextRenderer (FreeType), Cursor, HUD/InfoPanel, NameTag
│   └── shaders/ text.vs/fs, hud.vs/fs
├── app2d/                      # space_garden_2d: orthographic renderer of Garden
├── app3d/                      # space_garden_3d: perspective renderer of Garden
├── tests/                      # core_tests (plain asserts or doctest, run with ctest)
├── assets/ textures/ fonts/ models/ cursor/
└── docs/
```

Dependency rule (enforced by CMake target links):

```
garden_core  <-  garden_engine  <-  app2d / app3d
(no GL)          (GL, GLFW, FreeType)    (app3d also Assimp)
```

### Dependencies

| Library | How | Why |
|---|---|---|
| GLFW 3.4 | FetchContent | window, input, cursor |
| GLM 1.0.1 | `include/glm/` (header-only, per your rule; see D1) | math |
| FreeType 2.13 | FetchContent | text in info panel |
| Assimp 5.4 | FetchContent, app3d only, importers limited to OBJ/glTF | plant models |
| GLAD (GL 3.3 core) | `include/` + `third_party/glad/glad.c` | GL loader |
| stb_image | `include/stb_image.h` | textures, cursor image |
| LearnOpenGL `shader.h`, `camera.h`, `mesh.h`, `model.h` | `include/learnopengl/` | per your note |

Ubuntu prerequisites (one time):
```
sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev libwayland-dev libxkbcommon-dev
```

### Core model (the shared "logic")

```cpp
enum class Stage { Empty, Seed, Sprout, Mature };

struct Pot {
    Stage stage = Stage::Empty;
    float moisture = 0;      // 0..capacity
    float progress = 0;      // 0..1 growth
    std::vector<Droplet> droplets; // y in [0,1] = normalized height above soil
};

class Garden {
public:
    void select(int potIndex);            // -1 = none
    CommandResult apply(Command c);       // Sow, Water, Harvest, ToggleLamp, RefillTank
    void update(float dt);                // advances ALL pots independently
    const Pot& pot(int i) const; const Lamp& lamp() const; const Tank& tank() const;
    std::vector<Notice> notices() const;  // DrySoil, TankEmpty
};
```

Rules from the proposal, all in `update/apply`:
- Water: adds `min(waterPerPour, tank.water, capacity - moisture)`, only on Seed/Sprout, spawns droplets. Blocked if tank empty.
- Grow while `lamp.on && moisture > 0 && stage != Mature`; moisture decreases while growing; pauses without loss otherwise.
- Stage thresholds in `Config` (e.g. Sprout at 0.33, Mature at 1.0). Mature stops growing and drinking.
- Harvest only on Mature → Empty. Sow only on Empty.
- Droplets fall at constant speed and are removed at soil (y ≤ 0).

Renderers never change state directly — they only call `select()` / `apply()` and read state. Picking (which pot is under the mouse) is the renderer's job because it depends on the projection.

---

## Steps

Each step is one commit (sometimes two). "Verify" is what you check yourself before committing.

### Phase 0 — Foundation

**Step 1. Repo skeleton + CMake + deps**
- Top-level `CMakeLists.txt`, FetchContent for GLFW/FreeType, GLM in include/ (Assimp added later in step 17), `include/` with glad, stb_image, empty `learnopengl/`.
- `app2d/main.cpp` opens a window and clears to a dark blue color.
- `.gitignore` (build/), `README.md` with build commands.
- Verify: `cmake -B build && cmake --build build -j && ./build/app2d/space_garden_2d` shows a colored window.
- Commit: `build: cmake skeleton with fetched deps and glad`
- Notes: *GLFW & context*, *GLAD*.

**Step 2. Core logic library + tests (no graphics)**
- `garden_core` with Pot/Lamp/Tank/Garden/Config/Command, `core_tests` with ctest.
- Tests: sow→water→grow with lamp→stage transitions; growth pauses (lamp off / dry) without losing progress; tank empty blocks watering; pour clamped by tank and capacity; harvest resets; unselected pots still grow; droplets reach soil and disappear.
- Optional: `garden_cli` tiny console loop to play the logic by typing commands.
- Verify: `ctest --test-dir build` all green.
- Commit: `core: garden logic model with unit tests`

**Step 3. Engine: window requirements**
- `Window` class: fullscreen on primary monitor (`glfwGetVideoMode`), Esc closes anytime, 75 FPS frame limiter, delta time. VSync off so the limiter is what you see.
- Small FPS print to console (temporary).
- Verify: fullscreen, Esc quits, console shows ~75 FPS.
- Commit: `engine: fullscreen window, esc exit, 75 fps limiter`
- Notes: *Frame timing*.

**Step 4. Engine: shader class + first quad**
- `include/learnopengl/shader.h` (LearnOpenGL `Shader`), shaders loaded from `assets/shaders` (path set via a CMake compile definition so it works from any working directory).
- Draw one colored rectangle with VAO/VBO/EBO in app2d.
- Verify: rectangle drawn; editing the fragment shader color changes it with no recompile.
- Commit: `engine: shader class, first quad with VAO/VBO/EBO`
- Notes: *Pipeline*, *VBO/VAO/EBO*, *Shaders & GLSL*, *Uniforms*.

**Step 5. Engine: textures, blending, name tag**
- `Texture2D` loader (stb_image, flip-on-load, mipmaps), alpha blending enabled.
- Semi-transparent name tag texture in a corner ("Aleksa Ćetković, <index>") — required by spec.
- Verify: name tag visible and see-through over the background.
- Commit: `engine: texture loading, alpha blending, name tag overlay`
- Notes: *Textures*, *Blending*.

**Step 6. Engine: FreeType text renderer**
- LearnOpenGL Text Rendering chapter: glyph atlas per character, orthographic projection in pixels, `renderText(str, x, y, scale, color)`.
- Font in `assets/fonts/` (e.g. DejaVu Sans, supports Cyrillic/Latin diacritics if you want Serbian labels — see D4).
- Verify: text at several sizes/colors; the name tag could now be text instead of an image.
- Commit: `engine: freetype text rendering`
- Notes: *Text rendering*, *Orthographic projection*.

**Step 7. Engine: astronaut-glove cursor**
- Two images (open glove / pointing finger), `glfwCreateCursor` swapped on mouse press/release. Hotspot at the fingertip.
- Verify: cursor is the glove, changes on left click.
- Commit: `engine: custom glove cursor with click state`
- Decision D3 (cursor approach).

**Step 8. Engine: info panel (HUD)**
- `InfoPanel::draw(const Garden&)`: stage name, moisture/progress/water bars with %, lamp state, controls reminder, warnings (dry soil, empty tank). Bars = colored quads with a fill uniform; text via step 6.
- Lives in engine because both apps use it unchanged (3D draws it as a fixed 2D overlay).
- Verify: hook keys to the `Garden` in app2d and watch numbers/bars change even with no scene drawn yet.
- Commit: `engine: info panel bound to garden state`

### Phase 1 — 2D app

**Step 9. 2D scene layout**
- Orthographic projection in a fixed virtual resolution (e.g. 1920×1080) so layout is resolution-independent.
- `SpriteRenderer` (LearnOpenGL "Breakout" style: one quad VAO, model matrix per sprite).
- Cabinet front, back wall with space window (texture), three pots on the bottom, lamp above, tank on the side, panel area below.
- Verify: static scene looks right on your screen.
- Commit: `app2d: static cabinet scene with sprite renderer`
- Notes: *Transformations*, *Coordinate systems*.

**Step 10. 2D selection**
- Convert mouse pixels → virtual coords, AABB hit test vs pot rects → `garden.select(i)`. Colored frame around the selected pot.
- Verify: clicking each pot highlights it; clicking elsewhere deselects (or keeps — D5).
- Commit: `app2d: pot selection with mouse`

**Step 11. 2D keyboard actions + plant stages**
- Key → `Command` mapping (proposed: S sow, W water, H harvest, L lamp, R refill; D2).
- Stage sprites (seed, sprout, mature) per pot; `garden.update(dt)` every frame.
- Verify: full loop sow → water → lamp on → stages change → harvest. Other pots keep growing while not selected.
- Commit: `app2d: actions and growth stages`

**Step 12. 2D lamp, tank, droplets**
- Lamp on: bright lamp surface + semi-transparent beam quad (alpha blending, drawn after the pots).
- Tank: blue fill quad whose height = water ratio.
- Droplets: small sprites at positions mapped from core's normalized droplet height.
- Verify: beam is see-through; water level drops on watering; droplets fall at constant speed and vanish at soil; watering blocked when empty, refill works.
- Commit: `app2d: lamp beam, tank level and falling droplets`

**Step 13. 2D polish + requirement check** → tag `v2d`
- Go through the checklist at the bottom. README section with controls.
- Commit: `app2d: polish`, then `git tag v2d`.

### Phase 2 — 3D app

**Step 14. 3D bootstrap: camera on an arc**
- app3d target, perspective projection, `OrbitCamera` (yaw on a limited arc in front of the cabinet with ←/→, height with ↑/↓, always `lookAt` garden center). Mouse stays free.
- Draw a placeholder cube with depth test.
- Verify: camera moves only within limits and always faces the center.
- Commit: `app3d: perspective camera on limited arc`
- Notes: *Perspective projection*, *Camera / lookAt*, *Depth testing*.

**Step 15. 3D primitives + cabinet**
- Procedural mesh generators in engine/app3d: box, open box (cabinet: floor, sides, back, roof; front open), cylinder/hollow pot with visible soil disc, sphere (droplets). All with positions, normals, UVs, consistent CCW winding.
- Textures on the cabinet; space window on the back wall.
- Verify: cabinet looks closed except the front.
- Commit: `app3d: procedural meshes and cabinet`
- Notes: *Normals*, *Winding order*.

**Step 16. Depth test & face culling toggles**
- Keys (e.g. F1 depth, F2 cull) shown in the HUD. Spec: with depth on, toggling culling must not change the picture — so all meshes need correct winding and closed/visible-from-outside geometry (pot interiors need inward faces or double layer).
- Verify: toggle both; with depth on, culling on/off looks identical.
- Commit: `app3d: depth testing and face culling toggles`
- Notes: *Depth testing*, *Face culling*.

**Step 17. Lighting**
- Phong (Blinn-Phong) shader: lamp = point light (or spotlight pointing down, D6) that turns on/off with the garden lamp; weak ambient keeps the scene visible when off. Lamp mesh uses an emissive "light cube" shader.
- Verify: lamp off → dim scene; lamp on → pots lit from above, specular highlights move with camera.
- Commit: `app3d: phong lighting tied to lamp state`
- Notes: *Lighting*, *Normal matrix*.

**Step 18. Assimp plant models**
- Add Assimp via FetchContent (only OBJ/glTF importers on to keep build time down), `include/learnopengl/mesh.h` + `model.h`.
- Three stage models swapped by `pot.stage` (no growth animation). Fallback: procedural shapes if a model is missing.
- Verify: stages swap correctly and are lit by the lamp.
- Commit: `app3d: assimp stage models`
- Notes: *Model loading*.

**Step 19. 3D picking**
- Ray from mouse (inverse projection·view) tested against each pot's bounding cylinder/AABB → `garden.select(i)`. Highlight via emissive tint or outline.
- Verify: selection works from every camera position.
- Commit: `app3d: mouse ray picking of pots`
- Notes: *Ray picking*. Decision D7.

**Step 20. 3D tank window + droplets + HUD**
- Tank with a transparent window (drawn last, sorted) showing a water column scaled by water ratio.
- Droplet spheres falling above the selected watered pot (positions from core).
- Info panel + name tag reused from engine as the fixed 2D overlay (depth test off while drawing HUD).
- Verify: everything from 2D works in 3D; transparency looks right from all angles.
- Commit: `app3d: tank window, droplets, hud overlay`
- Notes: *Blending* (order of transparent objects).

**Step 21. 3D polish + requirement check** → tag `v3d`

---

## Requirement checklist (from course specs)

| Requirement | 2D | 3D | Step |
|---|---|---|---|
| C++, OpenGL ≥ 3.3, programmable pipeline only | ✔ | ✔ | 1, 4 |
| Shaders, uniforms | ✔ | ✔ | 4 |
| Transparency and/or textures | ✔ | ✔ | 5, 12, 20 |
| Mouse and/or keyboard input | ✔ | ✔ | 10, 11, 19 |
| Themed image cursor (glove) | ✔ | ✔ | 7 |
| Text via textures or FreeType | ✔ | ✔ | 6 |
| 75 FPS limiter, fullscreen, Esc exit | ✔ | ✔ | 3 |
| Semi-transparent name/index tag | ✔ | ✔ | 5 |
| Depth test & face culling toggles, no visual diff with depth on | — | ✔ | 16 |
| At least one light source | — | ✔ | 17 |
| Perspective projection | — | ✔ | 14 |
| Constant-speed droplets (accelerated also allowed) | ✔ | ✔ | 2, 12, 20 |

## Decisions taken

- **D1** GLM 1.0.1 copied into `include/glm` with the other header-only libraries.
- **D2** Keys: `S` sow, `W` water, `H` harvest, `L` lamp, `R` refill; `1`/`2`/`3` also select pots.
- **D3** Cursor: OS cursor via `glfwCreateCursor` (two 64×64 glove images, hotspot at the fingertip).
- **D4** UI language: English.
- **D5** Clicking empty space keeps the current selection (the panel stays useful); hovering a pot shows a faint frame.
- 2D layout uses a 1920×1080 virtual canvas letterboxed into the real framebuffer.
- Textures are drawn procedurally by `tools/generate_assets.py` (no third-party image licenses).
- Default tuning (`core/include/garden/Config.h`): pot capacity 100, pour 25, tank 300,
  growth 0.05/s (20 s to mature), drink 4/s.

## Open decisions (I'll ask you when we reach them)

- **D6** Lamp light type in 3D: point light vs spotlight cone (spotlight matches the 2D beam).
- **D7** 3D picking: math ray test (recommended) vs color-ID picking via an offscreen framebuffer.
- Tuning values in `Config` (growth speed, pour size, capacity) — chosen after step 11 by feel.
