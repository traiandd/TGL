# TGL

TGL is a small, archetype-based entity-component-system (ECS) game engine written in modern C++20 (`tgl/`). It was built as coursework for *Elements of Computer Graphics* at the Polytechnic University of Bucharest, on top of the department's teaching OpenGL framework (`framework/`), and is extracted here on its own so the engine code is easy to read and build in isolation.

This repository builds a small demo (`app/`) that boots the engine into an empty scene — a starting point for building something on top of `tgl/`, not a game.

## What's in `tgl/`

-   **Compile-time archetypes.** `Archetype<Components...>` (`tgl/archetype.hpp`) is a variadic-template view over an entity that statically guarantees a set of components exist, and mixes in per-component behavior via `ArchetypeExtender<Component, Derived>` specializations — so a type like `Camera3d : Archetype<Transform3dComponent, Camera3dComponent>` gets `SetPosition`, `RotateYaw`, etc. forwarded straight from its components with no vtables involved.
-   **Sparse-set component storage** (`tgl/component_data.*`) keyed by `EntityId`, with components registered via a `register_component(Type)` macro (`tgl/component/register_component.hpp`) that populates a runtime registry used when entities are spun up from prefabs.
-   **Scenes and systems** (`tgl/scene.hpp`, `tgl/system/`): a `Scene` owns component data, an active camera, and a list of `System`s; a singleton `GameManager` (`tgl/game_manager.hpp`) owns scenes by `SceneId`, drives the frame loop, and routes window/input callbacks into the active scene.
-   **Observer-based events** (`tgl/observer.hpp`) for update/input/mouse events, so systems subscribe instead of being hard-wired into the frame loop.
-   **Rendering**: a straightforward per-entity `Render3dSystem`, plus a `BatchRender3dSystem` that groups entities sharing a mesh/shader into GPU-instanced static batches (`tgl/core/batch.hpp`) for cheap large-count rendering.
-   **2D UI on top of a 3D world**: `Transform2dComponent`, `UIButtonComponent`/`button_click` system with screen-to-world picking, `UITextComponent` rendered through the framework's Freetype-based text renderer.
-   **Hierarchies and lifecycle**: `Children`/`Parent` components for transform hierarchies, `OnDelete` hooks, and a deferred delete queue flushed once per frame.
-   **Misc gameplay building blocks**: 2D hitboxes/collision, item hold/drag, prefabs (`tgl/prefab/`) as reusable entity templates.

## Repository layout

```
tgl/         the engine (this is the part worth reading)
framework/   OpenGL window/shader/mesh/resource layer TGL is built on
app/         minimal demo: boots the engine into an empty scene
assets/      shaders, primitive meshes, a font and a texture used by the demo
deps/        third-party headers + prebuilt Windows binaries (see LEGAL.txt)
```

`framework/` is adapted from the "GFX Framework", the MIT-licensed teaching OpenGL framework used by the Computer Graphics Department of the Polytechnic University of Bucharest (for the *Elements of Computer Graphics* and *Advanced Graphics Programming* courses) — it provides the window, GL object wrappers, resource loading, and input handling that `tgl/` is built on. Everything under `tgl/` and `app/` is original.

## Building

Requires a C++20 compiler and CMake 3.16+, plus GLEW, GLFW3, assimp, spdlog and Freetype.

-   Arch: `sudo ./tools/deps-arch.sh`
-   Ubuntu/Debian: `sudo ./tools/deps-ubuntu.sh`
-   Fedora: `sudo ./tools/deps-fedora.sh`
-   macOS: `./tools/deps-macos.sh` (requires [Homebrew](https://brew.sh/))
-   Windows: dependencies are already vendored in `deps/prebuilt/`

Then:

```sh
mkdir build && cd build
cmake ..
cmake --build .
```

Run the resulting `TGLDemo` binary from `build/bin/<Config>/`.

## License

MIT, see `LICENSE.md`. Third-party dependencies under `deps/` keep their own licenses, listed in `LEGAL.txt`.
