# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project overview

`simuladorArtilleria1` ("artillery simulator") is an Unreal Engine **5.8** C++ project (not currently a git repository). The C++ side is still just the default project boilerplate (`Source/simuladorArtilleria1/simuladorArtilleria1.{h,cpp}` only implements the primary game module — no custom Actors, Components, or gameplay classes exist yet). The only content asset is `Content/mapa.umap`, which is both the editor startup map and the packaged game's default map (`Config/DefaultEngine.ini`).

Key plugins enabled in `simuladorArtilleria1.uproject`:
- **CesiumForUnreal** — real-world geospatial 3D tiles/terrain. Its cache lives in `cesium-request-cache.sqlite` at the project root; if geospatial data looks stale, that cache (and the WAL/SHM files next to it) is the place to look.
- **ModelingToolsEditorMode** — editor-only mesh modeling tools (`TargetAllowList: Editor`, so it does not ship in packaged builds).

Input is set up for the **Enhanced Input** plugin (`DefaultPlayerInputClass`/`DefaultInputComponentClass` in `Config/DefaultInput.ini` point at `EnhancedInput` classes), but no Input Action/Mapping Context assets exist in `Content/` yet — that setup still needs to be created (typically as data assets, since there's no `Content/Input` folder today).

Engine is installed at `C:\Program Files\Epic Games\UE_5.8\`.

## Working with this codebase

There is no separate build/test tooling (no npm/cmake/make scripts) — everything goes through Unreal's own toolchain.

- **Regenerate IDE project files** (needed after adding/removing source files, since this repo doesn't auto-regenerate them):
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\GenerateProjectFiles.bat" "C:\Users\agrey\OneDrive\Documentos\Unreal Projects\simuladorArtilleria1\simuladorArtilleria1.uproject"
  ```
- **Build from the command line** (Development Editor config, used while iterating in-editor):
  ```
  "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" simuladorArtilleria1Editor Win64 Development -Project="C:\Users\agrey\OneDrive\Documentos\Unreal Projects\simuladorArtilleria1\simuladorArtilleria1.uproject" -WaitMutex
  ```
  Swap the target for `simuladorArtilleria1` (Game target, see `Source/simuladorArtilleria1.Target.cs`) to build a standalone/packaged-style binary instead of the editor.
- Otherwise, open `simuladorArtilleria1.sln` (or `.slnx`) in Visual Studio / Rider and build the `Development Editor` configuration — this is the normal day-to-day workflow, and what generates PDBs matching `Binaries/Win64/UnrealEditor-simuladorArtilleria1.dll`.
- Launch the editor directly against the project via `simuladorArtilleria1.uproject` (double-click, or `UnrealEditor.exe "<path>\simuladorArtilleria1.uproject"`).
- There are no automated C++ unit tests in this project yet. If adding any, Unreal's convention is the Automation Spec/Test framework (`IMPLEMENT_SIMPLE_AUTOMATION_TEST` or `FAutomationTestBase` subclasses), runnable from Editor Tools > Test Automation or via `-ExecCmds="Automation RunTests <TestName>"` on the command line.

## Notes for future changes

- New gameplay C++ classes belong under `Source/simuladorArtilleria1/`; remember to add the module to `PrivateDependencyModuleNames`/`PublicDependencyModuleNames` in `simuladorArtilleria1.Build.cs` when a new Engine module (Slate, OnlineSubsystem, etc.) is needed — see the commented-out examples already in that file.
- `Binaries/`, `Intermediate/`, `DerivedDataCache/`, and `Saved/` are all regenerated build/cache output — never hand-edit files there, and treat them as safe to delete if something is stale.
- The `.claude/skills/unreal-engine-cpp-pro` skill is installed locally and covers UObject hygiene and UE-specific performance patterns — worth invoking for non-trivial gameplay C++ work.
