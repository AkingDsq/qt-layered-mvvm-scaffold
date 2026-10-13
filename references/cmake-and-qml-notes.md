# CMake and QML notes

Details that the templates already encode. Read this when a configure or build step fails, or before changing the module layout.

## The plugin target rule

`qt_add_qml_module(<target> URI <uri> ...)` creates a second target that builds the QML plugin. From Qt's own `Qt6QmlMacros.cmake`:

```cmake
elseif(NOT DEFINED arg_PLUGIN_TARGET)
    if(arg_NO_CREATE_PLUGIN_TARGET)
        ...
    endif()
    set(arg_PLUGIN_TARGET ${target}plugin)
```

So the plugin target name is `<target>plugin`, derived from the **target**, not the URI. `qt_add_library({{QML_TARGET}} STATIC)` + `qt_add_qml_module({{QML_TARGET}} URI "{{QML_URI}}")` therefore produces `{{QML_TARGET}}plugin`, which is what `src/app/CMakeLists.txt` links. The URI and the target are deliberately different things: the URI (`{{QML_URI}}`) is the module name QML code imports, the target (`{{QML_TARGET}}`) is a CMake identifier.

Linking the plugin target, rather than the backing library, is what registers the QML types and pulls the module's resources into the executable. Keep the two names in sync: if the target is renamed but the executable still links the old `<name>plugin`, the link fails; if you drop the link entirely, the build succeeds and the app fails at runtime with "module not installed".

## Static Qt builds (WebAssembly, iOS): the import scan must see your QML

A shared desktop Qt loads QML modules from the `qml/` directory on disk at runtime, so an import that was never linked still resolves. A static Qt has no such directory: `QtQuick.Layouts` and `QtQuick.Controls.Basic` exist only if their plugin libraries were linked into the executable. That is why the same source tree can build and run under MinGW and fail in the browser.

Qt automates the linking. `qt_add_executable()` defers `qt6_finalize_target()` whenever CMake is 3.19 or newer, and the finalizer registered on the `Qt6::Qml` target calls `qt6_import_qml_plugins(<exe>)`. That runs `qmlimportscanner` and links each imported QML plugin whose `qmldir` declares a `linktarget` resolving to a `STATIC_LIBRARY` - for example `Qt6::qtquicklayoutsplugin` and `Qt6::qtquickcontrols2basicstyleplugin`, both of which exist as `lib*.a` under `qml/QtQuick/...` in a static Qt install. All of those targets are defined as soon as `find_package(Qt6 COMPONENTS Qml)` runs, because `Qt6QmlPlugins.cmake` globs and includes every `QmlPlugins/Qt6*Config.cmake`.

The scan is **per target** and covers only that target's own QML files. `_qt_internal_scan_qml_imports()` passes `-rootPath <target SOURCE_DIR>` plus `-qrcFiles <the target's _qt_generated_qrc_files>`. When the QML module lives in a separate static library - as it does in this scaffold - the executable has no `.qml` files under its source dir and no generated qrc of its own, so the scanner reports zero imports, nothing is linked, and the app dies at startup with `module "QtQuick.Controls.Basic" is not installed`.

Widen the scan root on the executable target. This is Qt's documented escape hatch for this exact case (see the comment above the `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` lookup in `Qt6QmlMacros.cmake`):

```cmake
set_target_properties({{APP_TARGET}} PROPERTIES
    QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS "${CMAKE_SOURCE_DIR}/src/qml"
)
```

The templates already contain this line. To confirm it took effect, inspect the scanner output at `<build>/.qt/qml_imports/<target>_conf.cmake`: `qml_import_scanner_imports_count` must be greater than 0. A count of 0 means both lists above were empty and nothing will be linked.

Alternative fix: put the QML module on the executable with `qt_add_qml_module(<exe> URI "..." QML_FILES ...)`. Then the generated qrc belongs to the executable, the scan sees the QML files with no extra property, and `<QML_TARGET>plugin` is no longer needed - at the cost of the View layer no longer being its own library.

If the scan still comes up empty - for instance when `Qt6::Qml` is reached only through a generator expression, which the finalizer cannot resolve at configure time - call the import explicitly after all `target_link_libraries` calls. It is idempotent and guarded against running twice:

```cmake
qt_import_qml_plugins({{APP_TARGET}})
```

## Qt versions and policies

Use `qt_standard_project_setup(REQUIRES <min-version>)` rather than hand-rolling `qt_policy` calls. `REQUIRES` does double duty: it declares the minimum Qt version (a hard `FATAL_ERROR` below it) and it becomes the policy baseline, so every Qt CMake policy introduced at or before that version is set to `NEW`. `REQUIRES 6.5` therefore enables QTP0001 (the `:/qt/qml/` QML resource prefix) but not QTP0004 (6.8). Qt's own deprecation message for `AUTO_RESOURCE_PREFIX` points at exactly this command.

If you do need one policy on its own, guard it on the flag Qt publishes rather than on a version comparison:

```cmake
if(QT_KNOWN_POLICY_QTP0001)   # Qt 6.5+
    qt_policy(SET QTP0001 NEW)
endif()
if(QT_KNOWN_POLICY_QTP0004)   # Qt 6.8+
    qt_policy(SET QTP0004 NEW)
endif()
```

Two traps worth knowing:

- **There is no `QT_VERSION` variable.** Qt sets `QT_VERSION_MAJOR`, `QT_VERSION_MINOR`, `QT_VERSION_PATCH` and `Qt6_VERSION`. A guard written as `if(QT_VERSION VERSION_GREATER_EQUAL 6.8)` is therefore always false, the `qt_policy` call never runs, and the only symptom is a developer warning - `Qt policy QTP0001 is not set ...` - that is easy to scroll past. The build still succeeds, with the policy silently not applied.
- `qt6_policy` raises `FATAL_ERROR "<policy> is not a known Qt policy. Did you include the necessary Qt module?"` for anything the loaded Qt modules did not register, so an unguarded `qt_policy(SET QTP0004 NEW)` breaks configure on Qt 6.5-6.7. `Qt6QmlConfig.cmake` is what sets `QT_KNOWN_POLICY_QTP0001` and `QT_KNOWN_POLICY_QTP0004`.
- `qt_standard_project_setup()` has two other side effects worth knowing: it turns on `CMAKE_AUTOMOC` and `CMAKE_AUTOUIC` (**but not `CMAKE_AUTORCC`**, which the templates therefore still set), and on Windows it sets `CMAKE_RUNTIME_OUTPUT_DIRECTORY`, so the executable lands in the **build directory root** (`build/appMyApp.exe`) rather than in its own subdirectory. On any non-Windows target - Linux, macOS and WebAssembly alike - the executable stays under `build/src/app/`. Qt does this so a deployed Windows executable sits next to the Qt DLLs it needs.

## Module output directory and resource prefix

Let the top level decide once, and let every module inherit it:

```cmake
set(QT_QML_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qml")   # must be absolute
```

Each `qt_add_qml_module` then writes to `<that>/<target path>` automatically, so no call needs `OUTPUT_DIRECTORY`, and none needs `RESOURCE_PREFIX`. This is the simplest arrangement and the one that cannot drift out of sync.

A per-module `OUTPUT_DIRECTORY` is the trap. Qt requires it to end with the module's **target path**, which is derived from the URI (dots become slashes) and not from the CMake target name:

```text
CMake Warning at Qt6QmlMacros.cmake:4596
  The chartagenttest_qml target is a QML module with target path ChartAgentTest.
  It uses an OUTPUT_DIRECTORY of .../chartagenttest_qml, which should end in the
  same target path, but doesn't. Tooling such as qmllint may not work correctly.
```

Omitting `OUTPUT_DIRECTORY` *without* setting `QT_QML_OUTPUT_DIRECTORY` is not a fix either: for a library module the default is the target's own binary directory, which again does not end in the target path, so the same warning comes back.

- The resource prefix `:/qt/qml/<URI>/` comes from QTP0001 and needs no manual `RESOURCE_PREFIX`. Nothing should hardcode it: in QML use paths relative to the `.qml` file (`source: "images/logo.png"`), and in C++ use `loadFromModule(URI, "Main")` instead of `load(QUrl("qrc:/..."))`.
- Do not call `QQmlEngine::addImportPath` for your own modules, and do not copy QML files by hand; both fight the module system.

## Loading the module from C++

`engine.loadFromModule("{{QML_URI}}", "Main")` resolves the type `Main` inside the module URI and requires:

- the URI string here to equal the `URI` in `src/qml/CMakeLists.txt`;
- a `Main.qml` listed in that module's `QML_FILES`.

`loadFromModule` replaces the old `engine.load(QUrl("qrc:/Main.qml"))` form. Under QTP0001 the file lives at `:/qt/qml/{{QML_URI}}/Main.qml`, so a hand-written `qrc:/Main.qml` URL no longer matches anything.

## Automatic compilation and include paths

- `CMAKE_AUTOMOC ON` runs `moc` for every source that contains `Q_OBJECT`. A header with `Q_OBJECT` that is not listed as a source of some target is never moc'd, and the build fails at link time with an unresolved `vtable`/`staticMetaObject` symbol - so every new `QObject` subclass must also be added to its layer's source list. Listing both the `.h` and the `.cpp` file, as the templates do, is the reliable habit.
- `target_include_directories(<lib> PUBLIC <dir>)` propagates downward. `domain` and `infrastructure` are `PUBLIC`, so `app` compiles `#include "logging/LogApi.h"` and `#include "model/AppModel.h"` without any extra include path of its own. That is why the includes are relative to each layer root rather than to the project root.
- `CMAKE_AUTORCC ON` is set, but `resources/resources.qrc` is attached explicitly with `target_sources` so the resource is owned by the executable that uses it.

## Windows and WebAssembly

- `qt_add_executable(... WIN32 ...)` suppresses the console window on Windows. Drop `WIN32` while debugging if you want `qDebug` output in a terminal.
- The root `CMakeLists.txt` branches on `EMSCRIPTEN` only to print a status line. A WebAssembly build additionally needs the emscripten toolchain and `C:/Qt/<ver>/wasm_multithread` (or `wasm_singlethread`) as `CMAKE_PREFIX_PATH`, plus `QT_HOST_PATH` pointing at a desktop Qt of the same version.

## Reaching resources from C++ and QML

`:/...` and `qrc:/...` address the same resource tree, but they are **not** interchangeable in code. Measured on Qt 6.12 with a resource compiled in, from a base URL of `qrc:/qt/qml/Probe/Main.qml`:

| Context | Use this | Measured behaviour |
| --- | --- | --- |
| QML `source` / `url` | `"images/logo.png"` (relative), or `"qrc:/qt/qml/<URI>/images/logo.png"` | relative resolves against the `.qml` file's own resource URL |
| QML, `":/..."` | **do not use** | `Qt.resolvedUrl(":/data.txt")` returns `qrc:/qt/qml/Probe/:/data.txt` - it is treated as a *relative* path, so it never reaches the resource root |
| `QFile` / `QDir` / `QImage(const QString &)` | `":/data.txt"` | `QFile(":/data.txt").exists()` is `true`; `QFile("qrc:/data.txt").exists()` is **`false`** |
| `QUrl` (`QQmlComponent`, `QQuickView::setSource`, ...) | `"qrc:/data.txt"` | `QUrl(":/data.txt").isValid()` is **`false`** (empty scheme); `QUrl("qrc:/data.txt")` is valid with scheme `qrc` |

Practical rules:

- Put assets next to the `.qml` files and reference them **relatively**, so no prefix ever appears in your code. Add them with the `RESOURCES` argument of `qt_add_qml_module` rather than by hand-copying, so they are compiled into the module's qrc.
- The real question is never the spelling, it is **which resource tree the file is in**. This project has two: the QML module's generated qrc (`:/qt/qml/<URI>/...`, prefix from QTP0001) and the hand-written `resources/resources.qrc` (`prefix="/"` → `:/<path>`). Pick the addressing form from the API you are calling, as in the table above.
- In C++ prefer `engine.loadFromModule(URI, "Main")` over assembling a `qrc:` URL by hand.
- Do not call `QQmlEngine::addImportPath` for your own modules.

## Common failure modes

| Symptom | Cause |
| --- | --- |
| `Target "{{QML_TARGET}}plugin" ... not found` | target/plugin name mismatch, or a space/dot in the target name |
| Configure succeeds, app exits -1 immediately | `objectCreationFailed` fired - the URI or the `Main` type name does not resolve |
| `module "QtQuick.Layouts" is not installed` in a WebAssembly build while desktop is fine | the per-target import scan did not see the QML files - see "Static Qt builds" above |
| `module "..." is not installed` at runtime | the executable does not link the QML module or its plugin |
| `QML debugging is enabled. Only use this in a safe environment.` in the browser console | a Debug build with `QT_QML_DEBUG`; informational, never the cause of a load failure |
| Unresolved `staticMetaObject` / `vtable` at link time | a `Q_OBJECT` file is missing from its target's source list |
| `qmllint` warns about unqualified access to `App` | expected for a context property; `App` is not a registered QML type, so tooling cannot see it |
| Configure fails on `add_subdirectory(tests)` | `BUILD_TESTS=ON` without a `tests/CMakeLists.txt`; the template ships an inert one |
