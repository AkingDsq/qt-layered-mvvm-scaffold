# {{PROJECT_NAME}}

使用 **分层架构 + MVVM 混合架构** 的 Qt 6 / QML 最小工程。

## 架构

两种架构在同一条依赖链上分工：

- **分层架构**决定「谁能看见谁」：`infrastructure` ← `domain` ← `app`，`qml` 只被 `app` 看见。
- **MVVM**决定「界面怎么和逻辑对话」：`qml` 是 View，`src/app/viewModel` 是 ViewModel，
  `src/domain/model` 是 Model。View 只通过 ViewModel 的属性和命令读写状态。

| 目录 | 角色 | 依赖 |
| --- | --- | --- |
| `src/qml/` | View：QML 模块 `{{QML_URI}}` | 仅 Qt Quick |
| `src/infrastructure/` | 基础设施：日志等横切能力 | Qt Core |
| `src/domain/model/` | Model：领域模型与业务规则 | Qt Core、infrastructure |
| `src/app/viewModel/` | ViewModel：向 View 暴露状态与命令 | domain、infrastructure |
| `src/app/main.cpp` | 组合根：组装对象图并注入 QML | 全部 |

依赖方向固定为单向，禁止反向引用（`domain` 不得包含 `#include` 自 `app`/`qml` 的头文件）。

## 目录结构

```text
{{PROJECT_NAME}}/
├── CMakeLists.txt
├── resources/resources.qrc
├── importedcontent/
├── tests/                     # -DBUILD_TESTS=ON 时才参与构建
└── src/
    ├── CMakeLists.txt
    ├── app/                   # 应用层
    │   ├── CMakeLists.txt
    │   ├── main.cpp           # 组合根
    │   └── viewModel/
    ├── domain/                # 领域层
    │   ├── CMakeLists.txt
    │   └── model/
    ├── infrastructure/        # 基础设施层
    │   ├── CMakeLists.txt
    │   └── logging/
    └── qml/                   # View 层
        ├── CMakeLists.txt
        └── Main.qml
```

## 构建

需要 Qt 6.5+（含 Quick / Qml 模块）与 CMake 3.21+。已用 Qt 6.12 / MinGW 实测：配置与编译均通过。

```sh
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x.x/gcc_64
cmake --build build
./build/src/app/{{APP_TARGET}}
```

Windows（MinGW 或 MSVC）：

```powershell
cmake -S . -B build -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/mingw_64 -G "MinGW Makefiles"
cmake --build build
.\build\{{APP_TARGET}}.exe
```

注意 **Windows 上可执行文件落在构建目录根部**（`build\{{APP_TARGET}}.exe`），不是 `build/src/app/`。
这是顶层 `qt_standard_project_setup()` 的行为：它在 WIN32 下设置 `CMAKE_RUNTIME_OUTPUT_DIRECTORY`，
让 exe 与将来部署的 Qt DLL 待在同一个目录。Linux / macOS 上仍在 `build/src/app/`。

QML 模块的生成物统一在 `build/qml/{{QML_URI}}/`（`qmldir`、`*.qmltypes`、插件库）。

运行前需要能找到 Qt 动态库：把 `<Qt>/bin` 加进 `PATH`，或用 `windeployqt` 把依赖部署到 exe 旁边。

若未显式传入 `CMAKE_PREFIX_PATH`，也可以先设置环境变量 `QT_DIR` 指向 Qt 安装目录。

## 新增一个 MVVM 特性

1. 领域层：在 `src/domain/model/<feature>/` 新增模型类，并加入 `src/domain/CMakeLists.txt`。
2. ViewModel：在 `src/app/viewModel/` 新增 `<Feature>ViewModel`，用 `Q_PROPERTY` + 信号暴露状态，
   用 `public slots` 暴露命令，并加入 `src/app/CMakeLists.txt`。
3. View：在 `src/qml/` 新增界面文件，绑定 ViewModel 的属性、调用其命令；
   必要时在 `main.cpp` 中把新的 ViewModel 注入根上下文。
4. 不要跨层访问：View 里不出现业务规则，Model 里不出现界面类型。
