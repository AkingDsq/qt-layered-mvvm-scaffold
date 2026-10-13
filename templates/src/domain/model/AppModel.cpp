#include "AppModel.h"

#include <QCoreApplication>

AppModel::AppModel(QObject *parent)
    : QObject(parent)
    , m_title(QCoreApplication::applicationName())
{
    // QObject 基类必须收到 parent，否则对象树与生命周期管理会失效
    if (m_title.isEmpty())
    {
        m_title = QStringLiteral("{{PROJECT_NAME}}");
    }
}

AppModel::~AppModel() = default;

QString AppModel::title() const
{
    return m_title;
}
