# TGL

TGL is a small, archetype-based entity-component-system (ECS) game engine written in modern C++20 (`tgl/`). It was built as coursework for *Elements of Computer Graphics* at the Polytechnic University of Bucharest, on top of the department's teaching OpenGL framework (`framework/`), and is extracted here on its own so the engine code is easy to read and build in isolation.

This repository builds a small demo (`app/`) that boots the engine into an empty scene — a starting point for building something on top of `tgl/`, not a game.

## What's in `tgl/`

-   **Compile-time archetypes.** `Archetype<Components...>` (`tgl/archetype.hpp`) is a variadic-template view over an entity that statically guarantees a set of components exist, and mixes in per-component behavior via `ArchetypeExtender<Component, Derived>` specializations — so a type like `Camera3d : Archetype<Transform3dComponent, Camera3dComponent>` gets `SetPosition`, `RotateYaw`, etc. forwarded straight from its components with no vtables involved.
-   **Archetype/table component storage** (`tgl/component_data.*`): entities with the same exact component set share a contiguous, columnar `ArchetypeTable`; `ForEach<Components...>` walks only the tables whose signature is a superset of the query, swap-removing rows on delete or structural change. Components are registered via a `register_component(Type)` macro (`tgl/component/register_component.hpp`) that populates a runtime registry used when entities are spun up from prefabs.
-   **Scenes, systems, and a window-free core** (`tgl/scene.hpp`, `tgl/scene_manager.*`, `tgl/system/`): a `Scene` owns component data, an active camera, and a list of `System`s. Scenes themselves live in a window-independent `SceneManager` singleton, so ECS/scene behavior can be exercised without a live window (see Testing below); a singleton `GameManager` (`tgl/game_manager.hpp`) sits on top, forwarding scene management to `SceneManager` while owning the window, driving the frame loop, and routing input callbacks into the active scene.
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
tests/       unit tests for tgl/'s ECS core (doctest)
bench/       microbenchmarks for tgl/'s ECS core (nanobench), incl. alternate implementations kept for comparison
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

## Testing & benchmarking

`tgl/`'s ECS core is unit-tested and benchmarked independently of the window/rendering stack, via two extra CMake targets (`TGL_BUILD_TESTS` / `TGL_BUILD_BENCHMARKS`, both `ON` by default) that link only the engine sources they need — no OpenGL, GLFW, assimp, or Freetype.

-   **`tgl_tests`** (`tests/`, [doctest](https://github.com/doctest/doctest)): covers `ComponentData`'s archetype storage — add/get/remove, table transitions, swap-remove row relocation, `ForEach` — plus `Scene`/`EntityInstance` resolving through `SceneManager` with no window involved. Run with `./tgl_tests` from the build directory, or via `ctest`.
-   **`tgl_bench`** (`bench/`, [nanobench](https://github.com/martinus/nanobench)): benchmarks `ComponentData`'s `GetComponent` / `AddComponent` / `ForEach` against two earlier implementations frozen for comparison under `bench/implementations/` — `dense_array` (a `TypeId`-indexed `vector` per component type) and `legacy` (the original `type_index` hashmap lookup). Run with `./tgl_bench`.

Both targets build alongside `TGLDemo` from the same `cmake --build .`; pass `-DTGL_BUILD_TESTS=OFF -DTGL_BUILD_BENCHMARKS=OFF` to skip them.

### Performance

A sample `tgl_bench` run (Release build, `-O3 -DNDEBUG`), 10,000 entities, `current` = this repo's archetype/table `ComponentData`:

| Operation                | current (archetype/table) | dense_array (`TypeId`-indexed vector) | legacy (`type_index` hashmap) |
| ------------------------ | -------------------------: | -------------------------------------: | ------------------------------: |
| `GetComponent`           |                    0.89 ns |                                 0.95 ns |                          6.41 ns |
| `AddComponent`           |                   12.74 ns |                                 4.15 ns |                         21.07 ns |
| `ForEach<T>`              |                    5.03 ns |                                 4.97 ns |                          5.11 ns |
| `ForEach<T, U>`           |                    8.77 ns |                                7.72 ns* |                        15.52 ns* |

\* `dense_array`/`legacy` have no native multi-component query; this is `ForEach<T>` plus a manual has-check for `U` on every row — the fallback either would need without archetype storage.

`current` and `dense_array` are essentially tied on `GetComponent` and single-component `ForEach` — both boil down to simple, fully-inlinable dense-array indexing, while `legacy`'s real hashmap lookup costs several times more either way. `AddComponent` shows the expected shape: `dense_array` is fastest since adding a component never touches any other component's storage, `current` pays a real cost for its per-call archetype table transition (relocating every component the entity already has), but still comes in well ahead of `legacy`'s hashmap-based dispatch. On the two-component `ForEach`, `current`'s native archetype query and `dense_array`'s manual has-check land close together — the clear, consistent gap is against `legacy`, which pays roughly double either of them for having no way to skip per-row lookups at all. The `current`-vs-`dense_array` margins move around a bit run to run (close enough to be noise); the stable signal across runs is `current` beating `legacy`'s hashmap cost everywhere, and paying a real, structural premium specifically on `AddComponent` for its table-transition design — the standard archetype-ECS trade (cheap, cache-friendly iteration; costlier structural changes), the same one [flecs' own docs](https://www.flecs.dev/flecs/md_docs_2FAQ.html) describe in a mature engine.

Measured with `tgl_bench` built in Release; the `GetComponent` block also raises nanobench's `minEpochIterations` to get a stable reading on an operation fast enough that the default settings flagged it as unreliable (see `bench/component_data_bench.cpp`). Still run on a laptop with CPU frequency scaling and turbo enabled, so treat absolute values as approximate — run `./tgl_bench` yourself (see Testing above for how to configure a Release build) for numbers specific to your machine.

## License

MIT, see `LICENSE.md`. Third-party dependencies under `deps/` keep their own licenses, listed in `LEGAL.txt`.
