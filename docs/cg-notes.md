# Computer Graphics Notes — Space Garden

Quick reference for concepts used in the project. Links point to the matching LearnOpenGL chapter.

## GLFW & context
GLFW creates the window, the OpenGL **context** (the GL state machine bound to that window), and gives input callbacks. We request **3.3 core profile**: deprecated fixed-function calls (`glBegin`, `glVertex`) don't exist, so everything must go through shaders. Fullscreen = pass `glfwGetPrimaryMonitor()` and the monitor's video mode to `glfwCreateWindow`.
https://learnopengl.com/Getting-started/Hello-Window

## GLAD
OpenGL functions live in the driver; their addresses must be loaded at runtime. GLAD generates a loader: call `gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)` after making the context current.

## Frame timing
- **Delta time**: seconds since the last frame; all logic (`garden.update(dt)`) uses it so speed doesn't depend on FPS.
- **75 FPS limiter**: target frame time = 1/75 s ≈ 13.33 ms. After rendering, sleep/spin until that time has passed. VSync (`glfwSwapInterval(1)`) would lock to the monitor rate instead, so it's off.
- **Double buffering**: draw into the back buffer, `glfwSwapBuffers` shows it.

## The rendering pipeline
Vertex data → **vertex shader** (per vertex, outputs clip-space position) → primitive assembly → rasterization (triangles → fragments) → **fragment shader** (per fragment, outputs color) → depth/stencil tests → blending → framebuffer. Vertex and fragment shaders are the programmable stages we write.
https://learnopengl.com/Getting-started/Hello-Triangle

## VBO / VAO / EBO
- **VBO** (Vertex Buffer Object): GPU memory holding vertex data (positions, UVs, normals).
- **VAO** (Vertex Array Object): remembers *how* to read the VBO — which attribute is at which offset/stride (`glVertexAttribPointer` + `glEnableVertexAttribArray`) and which EBO is bound. Bind the VAO, then draw.
- **EBO/IBO** (Element Buffer): indices so shared vertices are stored once (a quad = 4 vertices + 6 indices). Draw with `glDrawElements`.
- Interleaved layout example: `[x y z | nx ny nz | u v]`, stride = 8 floats.

## Shaders & GLSL
Small programs compiled at runtime (`glCompileShader`, `glLinkProgram`). `in`/`out` variables pass data from vertex to fragment shader; outputs are interpolated across the triangle. The LearnOpenGL `Shader` class wraps loading, compiling, error printing, and uniform setters.
https://learnopengl.com/Getting-started/Shaders

## Uniforms
Global shader values set from C++ per draw call (`glUniform*`): matrices, colors, fill amount of a bar, light on/off, time. Same for every vertex/fragment of that draw.

## Textures
Images sampled in the fragment shader via **UV coordinates** (0..1). Key settings: wrapping (`GL_REPEAT`, `GL_CLAMP_TO_EDGE`), filtering (`GL_NEAREST` pixelated vs `GL_LINEAR` smooth), **mipmaps** (smaller pre-filtered copies for distant/minified surfaces). stb_image loads PNG/JPG; flip vertically because image origin is top-left and GL's is bottom-left. Multiple textures → **texture units** (`glActiveTexture(GL_TEXTURE0 + i)`).
https://learnopengl.com/Getting-started/Textures

## Blending (transparency)
`glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA)` → `result = src*α + dst*(1−α)`. Used for the lamp beam, name tag, tank window. In 3D: draw opaque objects first, then transparent ones **sorted back-to-front**, otherwise depth test hides things behind glass.
https://learnopengl.com/Advanced-OpenGL/Blending

## Transformations
4×4 matrices from GLM: translate, rotate, scale. Order matters — applied right-to-left: `model = T * R * S` scales first, then rotates, then translates. Vectors use `w=1` for points, `w=0` for directions.
https://learnopengl.com/Getting-started/Transformations

## Coordinate systems
Local (object) → **model** → world → **view** → view/eye space → **projection** → clip space → perspective divide → NDC (−1..1) → viewport → screen pixels.
`gl_Position = projection * view * model * vec4(aPos, 1.0);`
https://learnopengl.com/Getting-started/Coordinate-Systems

## Orthographic projection (2D app, HUD, text)
`glm::ortho(left, right, bottom, top)` maps a box straight to NDC with no perspective — parallel lines stay parallel, size doesn't change with distance. We use a virtual resolution (e.g. 0..1920 × 0..1080) so coordinates are in "pixels" independent of the real monitor. Mouse picking in 2D = convert screen mouse coords to this space and test rectangles (AABB).

## Perspective projection (3D app)
`glm::perspective(fovY, aspect, near, far)` builds a frustum; after the divide by `w`, distant objects look smaller. Keep `near` not too small (e.g. 0.1) to preserve depth precision.

## Camera / lookAt
`glm::lookAt(eye, target, up)` builds the view matrix. Our camera is an **orbit camera**: position from spherical coords (yaw limited to an arc, height clamped), target fixed at garden center, so it always looks at the garden.
https://learnopengl.com/Getting-started/Camera

## Depth testing
Depth buffer stores each pixel's depth; a fragment is kept only if closer (`GL_LESS`). Enable with `glEnable(GL_DEPTH_TEST)` and clear with `GL_DEPTH_BUFFER_BIT` every frame. Turn it off for the HUD overlay.
https://learnopengl.com/Advanced-OpenGL/Depth-testing

## Face culling & winding order
Triangle vertices listed **counter-clockwise** (as seen from outside) = front face. `glEnable(GL_CULL_FACE); glCullFace(GL_BACK)` skips back faces → less work. Spec requirement: with depth testing on, culling on/off must look the same, which only holds if every mesh has consistent winding and no surface is meant to be seen from its back (e.g. pot interior needs its own inward-facing triangles).
https://learnopengl.com/Advanced-OpenGL/Face-culling

## Normals & normal matrix
Normal = unit vector perpendicular to the surface, needed for lighting. When the model matrix has non-uniform scale, transform normals with `transpose(inverse(mat3(model)))` (the **normal matrix**).

## Lighting (Phong / Blinn-Phong)
Color = **ambient** (constant weak light — keeps scene visible when lamp is off) + **diffuse** (`max(dot(N, L), 0)`, brighter facing the light) + **specular** (shiny highlight, depends on view direction; Blinn uses the half vector `H = normalize(L+V)`). **Point light** with attenuation `1/(c + l·d + q·d²)`; **spotlight** adds a cutoff cone (good for a ceiling lamp). Lamp off → set light intensity uniform to 0.
https://learnopengl.com/Lighting/Basic-Lighting · https://learnopengl.com/Lighting/Light-casters

## Text rendering (FreeType)
FreeType rasterizes each glyph to a bitmap; we upload each to a single-channel (`GL_RED`) texture, store size/bearing/advance, then draw one textured quad per character in ortho projection. Need `glPixelStorei(GL_UNPACK_ALIGNMENT, 1)`. The fragment shader uses the red channel as alpha.
https://learnopengl.com/In-Practice/Text-Rendering

## Model loading (Assimp)
Assimp imports OBJ/glTF into a node/mesh/material tree; LearnOpenGL's `Model` walks it and builds a `Mesh` (own VAO/VBO/EBO + textures) per sub-mesh. Rendering still goes through our shaders — Assimp only provides data.
https://learnopengl.com/Model-Loading/Assimp

## Ray picking (3D selection)
Mouse (pixels) → NDC (x, y in −1..1) → multiply by `inverse(projection)` → eye space ray → `inverse(view)` → world ray `origin + t·dir`. Test intersection with each pot's bounding box/cylinder, pick the nearest hit.
Alternative: **color picking** — render each pot with a unique flat color to an offscreen **framebuffer** and read the pixel under the mouse with `glReadPixels`.

## Custom cursor
`glfwCreateCursor(GLFWimage*, hotX, hotY)` makes an OS cursor from RGBA pixels (loaded with stb_image); the hotspot is the click point (fingertip). Alternative: hide the cursor and draw a textured quad at the mouse position in ortho projection.
