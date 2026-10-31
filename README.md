# Space Garden

A small space-station garden rendered with OpenGL 3.3 core (programmable pipeline only):
three pots, a shared lamp and a water tank. A 2D and a 3D application share one logic model.

Computer Graphics course project, FTN Novi Sad, 2025/2026 — Aleksa Ćetković, SV 77/2022.

## Requirements (Ubuntu)

```sh
sudo apt install build-essential cmake git libgl1-mesa-dev xorg-dev libwayland-dev libxkbcommon-dev
```

GLFW is fetched by CMake on the first configure. GLAD, GLM and stb_image are vendored in `include/`.

## Build and run

```sh
cmake -B build
cmake --build build -j
./build/app2d/space_garden_2d
```

## Documentation

- `docs/proposal.pdf` — project proposal
- `docs/implementation-plan.md` — step-by-step plan
- `docs/cg-notes.md` — computer graphics concepts reference
