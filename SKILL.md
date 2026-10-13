---
name: qt-layered-mvvm-scaffold
description: Scaffold a minimal Qt 6 / QML project that mixes layered architecture with MVVM - View in src/qml, ViewModel in src/app/viewModel, Model in src/domain/model, cross-cutting code in src/infrastructure, composition root in src/app/main.cpp. Use when the user wants a new Qt Quick C++ project, a layered + MVVM skeleton, or a "最小项目" in the style of the ChartAgentTest reference project.
whenToUse: The user asks to create or generate a new Qt 6 / QML C++ project structured with layered architecture plus MVVM (or to reproduce the ChartAgentTest layout).
---

# Qt layered + MVVM scaffold

The `templates/` directory in this skill is the source of truth. Generate a project by copying that tree and substituting placeholders. By default a generated file's final path is exactly its path under `templates/` (step 2 documents the single optional rename), so the tree below (relative to `templates/`) is also the expected output:

```text
CMakeLists.txt  .gitignore  README.md
resources/resources.qrc          importedcontent/README.md
tests/CMakeLists.txt
src/CMakeLists.txt
src/qml/{CMakeLists.txt,Main.qml}
src/infrastructure/CMakeLists.txt
src/infrastructure/logging/{LogApi.h,LogApi.cpp,LoggerService.h,LoggerService.cpp}
src/domain/CMakeLists.txt
src/domain/model/{AppModel.h,AppModel.cpp}
src/app/CMakeLists.txt  src/app/main.cpp
src/app/viewModel/{ApplicationControllerViewModel.h,ApplicationControllerViewModel.cpp}
```

21 files. Class and file names are fixed; only project identity is parameterized.

**Requirements:** Qt 6.5 or newer with the `Quick` and `Qml` modules, and CMake 3.21 or newer. Configure and build the generated project before reporting success. This scaffold was validated on Qt 6.12 by building a generated project with MinGW (46 build steps, no configure warnings caused by these templates, `qmlimportscanner` resolving 11 imports) and by building a real project with the same CMake configuration for WebAssembly with emscripten - there all 11 QML plugin archives, including `libqquicklayoutsplugin.a` and `libqtquickcontrols2basicstyleplugin.a`, end up in the `.wasm` link inputs. The generated executable has not been launched under automation; do that yourself when you need runtime proof. The root `CMakeLists.txt` calls `qt_standard_project_setup(REQUIRES 6.5)`, which sets both the minimum Qt version and the Qt policy baseline, including the `:/qt/qml/` QML resource prefix. Do not replace it with `qt_policy` calls guarded by a `QT_VERSION` comparison: Qt defines `QT_VERSION_MAJOR/MINOR/PATCH` and never `QT_VERSION`, so such a guard is always false and the policy silently stops applying. Raise `REQUIRES` to 6.8 if you add QML subdirectories that need QTP0004.

**Scope:** Qt Quick (QML) applications with a C++ backend. Not Qt Widgets, not console-only, not non-Qt.

## 1. Collect inputs

Reuse what the request already contains: if the user named the project, its display name, or the organisation, take those values and do not ask again. Ask once for the genuinely missing values, then proceed without further questions.

- `PROJECT_NAME` - PascalCase, usable as a CMake project name, a CMake target fragment and a QML module URI (e.g. `MyApp`).
- `APP_DISPLAY_NAME` - human-readable, used by `setApplicationName` and the window title (e.g. `My App`).
- `ORG_NAME` - `setOrganizationName` value (e.g. `MyCompany`). If the user has no preference, default to `PROJECT_NAME` instead of asking.
- Output directory, default `<workspace>/<PROJECT_NAME>`. If it exists and is non-empty, ask before writing into it.

If the name is not ASCII, or carries spaces, punctuation or a leading digit, keep the original as `APP_DISPLAY_NAME` and derive an ASCII identifier for `PROJECT_NAME` (e.g. "图表工具" / "Chart Tool" -> `ChartTool`). Say so when you do.

Derive these and state them back to the user:

| Placeholder | Value |
| --- | --- |
| `{{PROJECT_NAME}}` | `PROJECT_NAME` |
| `{{APP_DISPLAY_NAME}}` | `APP_DISPLAY_NAME` |
| `{{ORG_NAME}}` | `ORG_NAME` |
| `{{APP_TARGET}}` | `app` + `PROJECT_NAME` |
| `{{QML_TARGET}}` | lowercase `PROJECT_NAME` + `_qml`; a CMake target, so letters, digits and underscores only |
| `{{QML_URI}}` | `PROJECT_NAME`; the QML module URI that `import` and `loadFromModule` use |

`{{QML_TARGET}}` must be lowercase letters, digits and underscores only. If `PROJECT_NAME` has no usable lowercase form, ask for the target explicitly. The plugin target is `{{QML_TARGET}}plugin`; it follows from the target, so never substitute it directly.

## 2. Materialize

Enumerate the bundle with glob `templates/**/*` over this skill directory. For every file: read it, replace every placeholder occurrence, and write the result to `<output-dir>/<same relative path>`. Create the directories as needed.

Write file-by-file with the file tools - that always works and keeps each substitution auditable. If a shell is available, `Copy-Item -Recurse` / `cp -r` the tree first and then substitute in place is acceptable.

Do not rewrite the architecture while copying. The layering, target names, ownership conventions and comments in the templates are the deliverable.

The domain model is a neutral placeholder: `AppModel` (`src/domain/model/`), exposing a single read-only `title`. It deliberately carries no business meaning, so the generated project never claims to be about some other application's domain. It exists to keep the Model layer real and the MVVM chain exercised end to end. Replace it with your own concept, keeping the three conventions listed in its header comment. If you rename it, also update the three references - the source list in `src/domain/CMakeLists.txt`, the forward declaration in `ApplicationControllerViewModel.h`, and the `#include` plus construction in `ApplicationControllerViewModel.cpp`. The file count stays 21.

## 3. Verify before reporting

1. Glob `<output-dir>/**/*` and confirm all 21 paths exist.
2. Grep the output directory for `\{\{` - it must return zero matches. Any hit is an unfinished substitution.
3. Confirm the four cross-file couplings agree:
   - `src/qml/CMakeLists.txt` declares `qt_add_library({{QML_TARGET}} ...)` with `URI "{{QML_URI}}"`, and `src/app/CMakeLists.txt` links `{{QML_TARGET}}plugin`. Qt derives the plugin target from the **target**, not the URI (`set(arg_PLUGIN_TARGET ${target}plugin)`), so the pair must be `<QML_TARGET>` and `<QML_TARGET>plugin`.
   - the root `CMakeLists.txt` sets `QT_QML_OUTPUT_DIRECTORY` (absolute), and no `qt_add_qml_module` call passes `OUTPUT_DIRECTORY` or `RESOURCE_PREFIX`. A per-module `OUTPUT_DIRECTORY` has to end with the module's target path (derived from the URI) or Qt warns and `qmllint` degrades; letting the top level own it removes that whole class of mistake.
   - `engine.loadFromModule("{{QML_URI}}", "Main")` in `src/app/main.cpp` matches that URI and the `Main.qml` filename.
   - `src/CMakeLists.txt` adds `qml`, `infrastructure`, `domain`, `app` in that order.
   - `src/app/CMakeLists.txt` sets `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` to the View-layer directory. Without that line the project still builds and runs on desktop but fails on WebAssembly with `module "QtQuick.Controls.Basic" is not installed`, because the QML module sits in a separate target that the per-target import scan never visits. Mechanism and diagnosis in `references/cmake-and-qml-notes.md`.
4. If a build toolchain is reachable, configure and build, and report the exact command plus result:
   `cmake -S <output-dir> -B <output-dir>/build -DCMAKE_PREFIX_PATH=<Qt-install-prefix>` then `cmake --build <output-dir>/build`. `<Qt-install-prefix>` is the Qt install root that *contains* `lib/cmake/Qt6` (e.g. `C:/Qt/6.12.0/mingw_64`), never the `lib/cmake/Qt6` directory itself.
   A successful configure alone still proves CMake target wiring; a full build also proves the C++ and QML compile.
5. If no shell, compiler or Qt is available, say so plainly: report which checks ran and that the project was not compiled. Do not imply a build happened.

Acceptance criteria:

- All 21 files present (still 21 after any example-model rename from step 2), zero unresolved `{{...}}`.
- Single direction of dependency: `src/domain/**` and `src/infrastructure/**` contain no `#include` reaching into `app/`, `viewModel/` or QML, and no QML/Quick types; `src/qml/**` contains no business rules.
- The View reaches the ViewModel only through the `App` context property (`App.greeting`, `App.title`, `App.refreshGreeting`).
- Build outcome reported, or the reason it could not be run.
- If the user targets WebAssembly, say explicitly that the `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` line is what makes the static QML imports link, and that a desktop-only build does not exercise it.

## 4. After generating

Report the output path, the resolved placeholder values, the verification results, and the exact build command for the user's Qt installation.

For architecture rules, adding a feature, renaming the ViewModel or model, and WebAssembly / `windeployqt` deployment, read `references/architecture-contract.md`, `references/cmake-and-qml-notes.md` and `references/deployment-and-extension.md`. Read them only when the task needs that detail.
