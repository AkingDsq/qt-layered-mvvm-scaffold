#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>

#include <memory>

#include "logging/LogApi.h"
#include "logging/LoggerService.h"
#include "viewModel/ApplicationControllerViewModel.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("{{APP_DISPLAY_NAME}}"));
    app.setOrganizationName(QStringLiteral("{{ORG_NAME}}"));

    // 基础设施：在组合根创建日志服务，并交给 LogApi 门面，供各层解耦调用
    auto logger = std::make_shared<LoggerService>();
    logger->initialize(QCoreApplication::applicationDirPath());
    LogApi::setActiveLogger(logger);

    // ViewModel：通过根上下文属性 "App" 暴露给 QML，View 层只依赖这个契约
    ApplicationControllerViewModel appController;

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("App", &appController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []()
        { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    // 加载 View 层注册的 QML 模块（URI 由 src/qml/CMakeLists.txt 定义）
    engine.loadFromModule("{{QML_URI}}", "Main");

    if (engine.rootObjects().isEmpty())
    {
        return -1;
    }

    LOG_KEY_INFO("App", "{{PROJECT_NAME}} started");

    return app.exec();
}
