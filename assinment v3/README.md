# OpenGL 3D Card Game Assignment (HW-2026)

Complete C++20 / OpenGL 4.6 extension of the Lecture 5 project implementing all requirements of `HW-2026.pdf`.

## Project Architecture
All original Lecture 5 files (`PrimitivesMain.cpp`, `ModelsMain.cpp`, `MultilightMain.cpp`, `tools/Cube.*`, `tools/Polygon.*`, `shaders/*/L5*`, and `CMakeLists.original.txt`) are preserved unmodified.

### New / Updated Files
| File | Responsibility |
| :--- | :--- |
| `CMakeLists.txt` | Compiles `CardGameMain.cpp`, `GLProject/cardgame/*.cpp`, and `GLProject/tools/*.cpp` while excluding the 3 original `*Main.cpp` entry points to prevent duplicate `main()` symbols. |
| `GLProject/CardGameMain.cpp` | GLFW/GLAD setup, edge-triggered input (`1`, `2`, `G`, `ESC`), lighting uniforms, update/render loop. |
| `GLProject/cardgame/TextureLoader.h/.cpp` | Loads textures via `stb_image.h` (reusing `OpenGL.SharedModule/src/stb_image.cpp`), mipmaps, RGBA/RGB/RED support, and RAII cleanup. |
| `GLProject/cardgame/CameraSystem.h/.cpp` | Manages Camera 1 (Top-Down) and Camera 2 (Player View) with cubic smoothstep transitions and card stand-up blend factor. |
| `GLProject/cardgame/CardDeck.h/.cpp` | Reuses Lecture 5's `PolygonWithTexture` and `Cube` for 5 square cards (cartoon portraits: Radwan, Michel, Antoun, Jad, Mr.Beast "The Millionaire"), non-duplicate random draws on `G` (ignored after 5 draws), parabolic flight animation, and 0°↔90° stand/flatten animation. |
| `GLProject/cardgame/RoomEnvironment.h/.cpp` | Spacious 12×6×12 3D room (wood floor, wallpaper walls, ceiling, baseboards, lamp), gaming table with 3 board sections (Opponent [empty], Player Played, Player Deck), torn wall poster showing the mate-cup photo (`PolygonWithTexture` `GL_TRIANGLE_FAN` with jagged missing top-right corner), and 3 decorative objects (Porcelain Teapot + saucer with animated steam [procedural lathe/sweep mesh, `tools/ProceduralMesh`], Damascene Sword monument [procedural mesh textured with the reference photo, `tools/ProceduralMesh`], Display Pedestal + 3D Model via Lecture 5's `Model` class). |
| `GLProject/cardgame/DecorScoring.h/.cpp` | Inverse Document Frequency (IDF) originality scoring calculator and max-1-model validator. |
| `GLProject/shaders/vs/CardGame.vs` & `fs/CardGame.fs` | World-space Blinn-Phong multi-light shaders supporting both `Polygon`/`PolygonWithTexture`/`Cube` (`dFdx`/`dFdy` flat normals) and Assimp `Model` smooth vertex normals. |

## Mathematical & Interview Reference
### 1. Card Orientation & Flight Transformations
Each card's front face is pivoted along its **bottom edge (`z = 0`)** in local space so rotating around `+X` lifts it upright without clipping into the table:
$$M_{\text{card}} = T(\mathbf{p}(t)) \cdot R_x(\theta) \cdot S(s(t))$$
- **Camera 1 (Top View):** $\theta = 0^\circ$ (card lies flat on the tabletop, front normal $+Y$).
- **Camera 2 (Player View):** $\theta = +90^\circ$ (card stands vertically on its bottom edge, front normal $+Z$ facing the player).
- **Card Flight (`G` key):** Cubic ease-in-out horizontal interpolation from Deck slot to Played slot plus parabolic lift $\Delta y = 0.44 \sin(\pi t)$ and subtle mid-flight scale boost $s(t) = 1 + 0.12 \sin(\pi t)$.

### 2. Decorative Object IDF Formula
$$\text{IDF}(d) = \ln\left(\frac{N}{df_d}\right), \qquad \text{Normalized Weight}(d) = \frac{\text{IDF}(d)}{\ln(N)}$$
where $N$ is the total number of teams and $df_d \in [1, N]$ is the number of teams using decorative object $d$. Common objects (high $df_d$) receive lower weight; unique objects ($df_d = 1$) receive weight $1.0$.

## Build & Run Instructions
```bash
cmake -S . -B build
cmake --build build
```
Run the compiled `GLProject` binary from the `build/` output directory (CMake automatically syncs `GLProject/` shaders, textures, and models to the target output directory).

## Controls
- **`1`** — Switch to Camera 1 (Top-Down View, cards lie flat).
- **`2`** — Switch to Camera 2 (Player Perspective View, cards stand vertically).
- **`G`** — Randomly draw 1 remaining card from Player Deck to Player Played area (works in both camera modes; 6th+ press does nothing).
- **`ESC`** — Exit.
