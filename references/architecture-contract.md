# Architecture contract

Two architectures share one dependency chain. The layering answers "who may see whom"; MVVM answers "how the UI talks to logic". Both are enforced by the CMake target graph, not by convention alone.

## Layers, roles and dependencies

| Layer | Directory | MVVM role | CMake target | May depend on |
| --- | --- | --- | --- | --- |
| View | `src/qml/` | View | `{{QML_TARGET}}` (STATIC + QML module, URI `{{QML_URI}}`) and its plugin `{{QML_TARGET}}plugin` | Qt Quick, Qt Qml |
| Infrastructure | `src/infrastructure/` | - (cross-cutting) | `infrastructure` (STATIC) | Qt Core |
| Domain | `src/domain/` | Model | `domain` (STATIC) | Qt Core, `infrastructure` |
| Application | `src/app/` | ViewModel + composition root | `{{APP_TARGET}}` (executable) | `{{QML_TARGET}}plugin`, `domain`, `infrastructure` |

Rules:

- Dependencies point one way only: `app` -> `domain`/`infrastructure`/`qml`, `domain` -> `infrastructure`. Nothing depends on `app`.
- The Domain layer may use `infrastructure` (the `LogApi` facade) but must not know about Qt Quick, QML, or any ViewModel.
- The View layer holds no business rules and no C++ includes. It reads and writes only through ViewModel properties and commands.
- Only `src/app/main.cpp` knows every layer at once. It is the single composition root: create services, create ViewModels, inject them into QML, then load the module.

`domain` linking `infrastructure` is a deliberate choice of this scaffold rather than strict Clean Architecture. It exists so the domain layer can log through the `LogApi` facade without a DI framework, at the cost of a compile-time edge from domain to infrastructure. If you later need the strictly inverted form, invert it by declaring an abstract logging interface in `domain` and implementing it in `infrastructure`; the rest of the tree does not change.

## MVVM wiring in the generated project

1. `AppModel` (`src/domain/model/`) is the Model. It is a neutral placeholder with no business meaning - the scaffold ships it so the layer is real and the chain is exercised, not because the generated project has a domain yet. It derives from `QObject` publicly so Qt's object tree owns it, and it exposes plain getters only.
2. `ApplicationControllerViewModel` (`src/app/viewModel/`) is the ViewModel. It owns the Model as a QObject child (`new AppModel(this)`), exposes read-only state as `Q_PROPERTY` (`NOTIFY` for values that change, `CONSTANT` for values fixed after construction), and exposes commands as `public slots`.
3. `main.cpp` calls `engine.rootContext()->setContextProperty("App", &appController)`. `App` is the whole View-to-ViewModel contract.
4. `Main.qml` binds one-way (`App.greeting`, `App.title`) and invokes commands (`onClicked: App.refreshGreeting()`). No `Connections`, no business logic in QML.

Because the ViewModel is exposed as a context property, every QML file in the module sees it. Prefer that for one top-level controller; for per-screen ViewModels, register them as QML types with `qmlRegisterType` or `QML_ELEMENT` instead and construct them in QML.

## Invariants that keep MVVM honest

- Every readable `Q_PROPERTY` either has a `NOTIFY` signal that its mutator emits, or is declared `CONSTANT` because the value is fixed after construction. A property with neither goes stale silently in QML.
- State is initialized before the constructor returns, so the first QML binding never reads an empty value.
- ViewModels never include QML headers and never touch `QQuickWindow`/`QQmlEngine` beyond what `main.cpp` does.
- Model objects are never passed to QML as raw pointers; the ViewModel projects them into plain values (`QString`, numbers, lists).

## Adding a feature

1. Domain: add `src/domain/model/<Feature>.h/.cpp` (or a `<feature>/` subdirectory when a group of models belongs together), then add those files to the `add_library(domain STATIC ...)` list in `src/domain/CMakeLists.txt`.
2. ViewModel: add `src/app/viewModel/<Feature>ViewModel.h/.cpp`, register both in the `qt_add_executable` source list in `src/app/CMakeLists.txt`, expose state via `Q_PROPERTY` + signals and commands via `public slots`.
3. Inject it in `main.cpp` (`setContextProperty` or `qmlRegisterType`).
4. View: add the QML file to the `QML_FILES` list in `src/qml/CMakeLists.txt` - a QML file that is not listed there is not compiled into the module and will not resolve at runtime.
5. Tests: link `domain` (and `infrastructure`), never `app` or the QML module.

## Renaming the ViewModel or the Model class

File names and class names are fixed by the templates. To rename one:

- Rename the header/source files on disk.
- Update the file names in the owning layer's `CMakeLists.txt`.
- Update the `#include` path in the two places that use it: `src/app/main.cpp` (for the ViewModel) and `src/app/viewModel/ApplicationControllerViewModel.cpp` (for the Model).
- Keep the include guard macros in step with the file name; a stale guard silently breaks re-inclusion semantics.
