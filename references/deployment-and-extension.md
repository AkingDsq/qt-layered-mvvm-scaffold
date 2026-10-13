# Deployment, extension and deviations

## Windows deployment with windeployqt

The reference project carries a `windeployqt` locator in the root `CMakeLists.txt`. It is deployment plumbing, not architecture, so it is kept out of the minimal scaffold. Add this after `find_package(Qt6 ...)` when you need to ship a Windows build:

```cmake
if(WIN32)
    # 优先使用缓存变量，允许用户在配置时覆盖
    set(WINDEPLOYQT "" CACHE FILEPATH "Path to windeployqt6 executable")

    if("${WINDEPLOYQT}" STREQUAL "")
        if(Qt6_HOST_BINS)
            set(WINDEPLOYQT "${Qt6_HOST_BINS}/windeployqt6.exe" CACHE FILEPATH "Path to windeployqt6 executable" FORCE)
        else()
            find_program(WINDEPLOYQT_PROG
                NAMES windeployqt6 windeployqt
                PATHS
                    "$ENV{QT_DIR}/bin"
                    "$ENV{QT_PREFIX}/bin"
                    "C:/Qt/*/msvc2022_64/bin"
                    "D:/Qt/*/msvc2022_64/bin"
                DOC "Path to windeployqt6 executable"
            )
            if(WINDEPLOYQT_PROG)
                set(WINDEPLOYQT "${WINDEPLOYQT_PROG}" CACHE FILEPATH "Path to windeployqt6 executable" FORCE)
            else()
                set(WINDEPLOYQT "windeployqt6" CACHE FILEPATH "Path to windeployqt6 executable" FORCE)
            endif()
        endif()
    endif()
endif()
```

Then add a `POST_BUILD` step on the executable target:

```cmake
if(WIN32)
    add_custom_command(TARGET {{APP_TARGET}} POST_BUILD
        COMMAND "${WINDEPLOYQT}" --no-translations "$<TARGET_FILE:{{APP_TARGET}}>"
        COMMENT "Deploying Qt runtime next to ${APP_TARGET}"
    )
endif()
```

Do not hard-code the Qt version into the deploy path; `--qmlimport`/`--qmldir` are only needed when the QML module is not built by this project, which it is here.

## WebAssembly

```powershell
cmake -S . -B build-wasm `
  -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/wasm_multithread `
  -DQT_HOST_PATH=C:/Qt/6.12.0/mingw_64 `
  -G "Ninja"
cmake --build build-wasm
```

Notes that matter for this scaffold:

- **Static Qt needs the QML imports linked in, and the default scan cannot see a QML module that lives in another library.** `QtQuick.Layouts` / `QtQuick.Controls.Basic` load from disk on a desktop build but simply do not exist in a static build unless `qt6_import_qml_plugins()` links them. That finalizer scans only the executable's own QML files, so with the View layer in a separate static library the scan finds nothing. The templates handle this with `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` on the executable target; see `cmake-and-qml-notes.md` for the mechanism and for how to confirm it from `<build>/.qt/qml_imports/<target>_conf.cmake`. Verify the result by checking that every QML plugin archive the scan lists appears in the link inputs, e.g. `libqquicklayoutsplugin.a` and `libqtquickcontrols2basicstyleplugin.a` in `build.ninja`.
- **A multithreaded wasm build requires cross-origin isolation.** The `wasm_multithread` Qt kit links with `-pthread`, pthreads need `SharedArrayBuffer`, and browsers expose `SharedArrayBuffer` only to a cross-origin-isolated page. The host must send `Cross-Origin-Opener-Policy: same-origin` and `Cross-Origin-Embedder-Policy: require-corp`; without them the page simply fails or hangs during wasm startup. Netlify and Cloudflare Pages read a `_headers` file, Vercel uses `vercel.json`, nginx uses `add_header`. **GitHub Pages cannot send custom headers**, so a multithreaded build will not run there - use another host or rebuild against a `wasm_singlethread` kit.
- The deploy set is small: `.html` (entry page), `.js` (emscripten glue), `.wasm`, `qtloader.js`, `qtlogo.svg` from `build/src/app/` - and nothing else (skip `cmake_install.cmake`).
- Size, measured on a Qt Quick app of this shape: Debug `.wasm` 41.05 MB, Release `.wasm` 24.78 MB (`.js` 0.51 MB -> 0.26 MB). Use `-DCMAKE_BUILD_TYPE=Release` (or `MinSizeRel` for `-Os`) before publishing; static hosts brotli/gzip `.wasm`, so the transfer size is roughly a third of that. Cloudflare Pages caps a single file at 25 MiB, which a Debug build exceeds outright and a Release build only just fits under.
- `QT_HOST_PATH` must reference a desktop Qt of the **same version**, built with the same generator you use for the host tools. Qt's `qt.toolchain.cmake` also locates the emscripten toolchain through the `EMSDK` environment variable, so activate the SDK (`emsdk_env`) before configuring.
- `LoggerService::initialize(QCoreApplication::applicationDirPath())` writes into the emscripten in-memory filesystem. Logs are lost on reload unless you flush them to persistence; that is fine for a scaffold but not for production telemetry.
- The Qt Creator kit name in the reference project (`WebAssembly Qt 6.12.0 multi threaded Debug`) has no effect on CMake; the toolchain file is what selects the target.

## Unit tests

`tests/CMakeLists.txt` is inert so that `-DBUILD_TESTS=ON` always configures. To add Google Test, the commented block that the reference project left in its root `CMakeLists.txt` works:

```cmake
option(ENABLE_UNIT_TESTS "Enable Google Test for unit tests" OFF)

if(ENABLE_UNIT_TESTS)
    include(FetchContent)
    FetchContent_Declare(googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
    )
    if(WIN32)
        set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    endif()
    FetchContent_MakeAvailable(googletest)
endif()
```

Link test targets against `domain`, `infrastructure` and `GTest::gtest_main` only. A test that needs `app` means the logic under test is in the wrong layer.

## importedcontent

`importedcontent/` is the drop point for design-to-code output (for example Qt's Figma import). Nothing references it by default, so files placed there are **not** built. Add `add_subdirectory(importedcontent)` to `src/CMakeLists.txt` when you actually put a `CMakeLists.txt` there.

## Logging

`infrastructure/logging` is a two-part design and should stay that way:

- `LoggerService` is the implementation. It is thread-affine: `append()` and `clear()` check `QThread::currentThread() == thread()` and use `QMetaObject::invokeMethod(..., Qt::QueuedConnection)` from foreign threads, so `m_entries` is only mutated on the owning thread. Every public accessor takes `m_mutex`.
- `LogApi` is the facade: a process-wide `std::shared_ptr<LoggerService>` behind a mutex, plus `LOG_KEY_INFO` / `LOG_KEY_WARNING` / `LOG_KEY_ERROR` macros. Callers never hold the service.

The service must be created and registered once, in the composition root, before anything logs. `LogApi` calls before `setActiveLogger` are silently dropped - that is intended, so library code can log unconditionally.

Growing this: add new infrastructure services the same way (concrete `QObject` service + thin facade), and inject or register them in `main.cpp` only.

## Deviations from the ChartAgentTest reference project

The scaffold mirrors the reference project's architecture, target graph and file layout, with these deliberate corrections. Each was verified against the reference sources; keep them unless you have a reason not to.

Identifier note: the reference project was later renamed so that every name derives from the directory name - CMake project `ChartAgentTest`, executable `appChartAgentTest`, QML library target `chartagenttest_qml` with module URI `ChartAgentTest` (plugin target `chartagenttest_qmlplugin`), and the ViewModel class corrected to `ApplicationControllerViewModel`. The rows below keep the identifiers the reference used when this scaffold was derived, so `3DEleSimulationWebPlatform` / `appEMS` / `ems_qml` / `ApplicationControllorViewModel` / `a'sa's` map onto those new names.

| Reference | Scaffold | Why |
| --- | --- | --- |
| `class DesktopAppProjectManager : QObject` (private inheritance) and a constructor that ignores `parent` | `AppModel` derives `public QObject` and forwards `parent`; the scaffold ships this neutral placeholder instead of the reference's project-specific model | Private `QObject` inheritance breaks `connect`, `qobject_cast` and the object tree; a `QObject` that drops its `parent` leaks and is not destroyed by its owner. The reference's domain model itself is application code, so it is not part of a generic scaffold. |
| `infrastructure` links `Qt6::Core Qt6::Quick Qt6::Qml` and adds `${CMAKE_SOURCE_DIR}/src/domain/model` to its include path | `infrastructure` links `Qt6::Core` only, no domain include path | The infrastructure layer contained no Quick/QML usage, and pointing it into `domain` created a cycle in the include graph while `domain` already links `infrastructure`. |
| `windeployqt` locator inline in the root `CMakeLists.txt` (~40 lines) | Moved to this reference file | Deployment plumbing, not part of a minimal architecture sample. |
| `option(BUILD_TESTS)` + `add_subdirectory(tests)` with no `tests/` directory | `tests/CMakeLists.txt` shipped (inert) | The reference fails to configure with `-DBUILD_TESTS=ON`. |
| `project(appEMS)` nested inside `src/app/CMakeLists.txt`, with the executable named `${PROJECT_NAME}` | `qt_add_executable({{APP_TARGET}} ...)` | A nested `project()` silently rewrites `PROJECT_NAME` for that scope; an explicit target name is clearer and rename-safe. |
| `Main.qml` never touched the ViewModel; `ApplicationControllerViewModel` was an empty `QObject` | `Main.qml` binds `App.greeting` / `App.title` and calls `App.refreshGreeting()`; the ViewModel exposes them and reads the model | The reference proved the wiring compiled but not that it worked. The scaffold exercises all four layers on first run, which also makes a broken injection fail visibly. |
| `app.setApplicationName("a'sa's")` | `{{APP_DISPLAY_NAME}}` | Placeholder test string. |
| `importedcontent/README.md` claimed the root `CMakeLists.txt` picks the directory up automatically | Corrected wording | Nothing referenced it. |
| Commented-out `Q_IMPORT_QML_PLUGIN(ems_qmlPlugin)` in `main.cpp` | Removed | Not needed when the executable links the module's plugin target, and the capitalisation did not match the real target. Set it only if you build the QML module as a truly static plugin and do not link the plugin target. |
| No scan root for the View directory on the executable; `# qt_import_qml_plugins(${PROJECT_NAME})` left commented out | `QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS` set on the executable target | The reference builds and runs with MinGW but fails under Qt for WebAssembly with `module "QtQuick.Controls.Basic" is not installed`. With a static Qt the QML imports must be linked in, the finalizer's scan only covers the executable's own QML files, and the QML lives in the separate `ems_qml` target - so the scan finds nothing. See `cmake-and-qml-notes.md`. |
