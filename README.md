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
-   **`tgl_bench`** (`bench/`, [nanobench](https://github.com/martinus/nanobench)): benchmarks `ComponentData`'s `GetComponent` / `AddComponent` / `ForEach` against two earlier implementations frozen for comparison under `bench/implementations/` — `dense_array` (a `TypeId`-indexed `vector` per component type) and `legacy` (the original `type_index` hashmap lookup) — plus three real third-party ECS libraries vendored under `deps/api/` and benchmarked via their own native APIs (never through a wrapper, so the numbers reflect what each library actually costs): [entt](https://github.com/skypjack/entt) v4.0.0 (sparse-set storage), [flecs](https://github.com/SanderMertens/flecs) v4.1.6 (archetype/table storage), and [gaia-ecs](https://github.com/richardbiely/gaia-ecs) v0.9.2 (archetype/chunk storage). Run with `./tgl_bench`.

Both targets build alongside `TGLDemo` from the same `cmake --build .`; pass `-DTGL_BUILD_TESTS=OFF -DTGL_BUILD_BENCHMARKS=OFF` to skip them.

### Performance

A `tgl_bench` run (Release build, `-O3 -DNDEBUG`), 10,000 entities, `current` = this repo's archetype/table `ComponentData`, compared against two frozen earlier implementations (`dense_array`, `legacy`) and three real third-party ECS libraries used via their own native APIs:

**Note**: These benchmarks are nowhere near enough to actually say that this is "better" than flagship ECS storage engines. TGL ECS has nowhere near the features these bring, and that might be the reason behind this speed. More benchmarks will need to be done, measuring more real scenarios, especially if TGL ECS will be more feature rich. 

| Operation                                  |  current | dense_array | legacy    | entt    | flecs                 | gaia-ecs              |
| ------------------------------------------- | -------: | -----------: | --------: | -------: | ---------------------: | ---------------------: |
| `GetComponent`                              |  0.74 ns |      0.96 ns |   6.84 ns |  2.16 ns |                3.22 ns |                2.10 ns |
| `AddComponent`                               |  7.44 ns |      2.20 ns |  17.04 ns |  5.59 ns | 67.99 ns / 37.58 ns\*  | 44.67 ns / 10.24 ns\*  |
| `ForEach<T>` (shared accumulator)            |  0.43 ns |      1.18 ns |   1.47 ns |  0.44 ns |                0.44 ns |                0.46 ns |
| `ForEach<T>` (independent per-entity writes) |  0.19 ns |      1.25 ns |   1.56 ns |  0.43 ns |                0.20 ns |                0.22 ns |
| `ForEach<T, U>`                              |  0.44 ns |      3.15 ns† |  11.91 ns† |  1.66 ns |                0.45 ns |                0.46 ns |

**Note**: `current` is not using compile time information in the `GetComponent` tests. If it were, the component existence check would not need to exist, dropping the `GetComponent` time to ~0.54ns. However, it is an unfair comparison against the other libraries since they do not have compile time component information on entities.

\* naive per-component `set<T>`/`add<T>` calls vs. each library's bulk-creation API (`ecs_bulk_init` for flecs, `copy_n` for gaia) — see the analysis below.

† `dense_array`/`legacy` have no native multi-component query; this is `ForEach<T>` plus a manual has-check for `U` on every row, the fallback either needs without archetype storage.

Measured with `tgl_bench` built in Release; the `GetComponent` block also raises nanobench's `minEpochIterations` to get a stable reading on an operation fast enough that the default settings flagged it as unreliable (see `bench/component_data_bench.cpp`). Still run on a laptop with CPU frequency scaling and turbo enabled, so treat absolute values as approximate — run `./tgl_bench` yourself (see Testing above for how to configure a Release build) for numbers specific to your machine.

## License
PolyForm Small Business License 1.0.0, see `LICENSE.md`. Third-party dependencies under `deps/` keep their own licenses, listed in `LEGAL.txt`.
