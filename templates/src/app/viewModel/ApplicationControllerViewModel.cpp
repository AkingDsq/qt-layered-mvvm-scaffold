#include "ApplicationControllerViewModel.h"

#include "logging/LogApi.h"
#include "model/AppModel.h"

ApplicationControllerViewModel::ApplicationControllerViewModel(QObject *parent)
    : QObject(parent)
    , m_model(new AppModel(this))
{
    LOG_KEY_INFO("ViewModel", "ApplicationControllerViewModel created");

    // 初始状态：构造函数结束前状态必须可用，避免 QML 首次绑定读到空值
    m_greeting = QStringLiteral("Hello from the ViewModel");
}

ApplicationControllerViewModel::~ApplicationControllerViewModel() = default;

QString ApplicationControllerViewModel::greeting() const
{
    return m_greeting;
}

QString ApplicationControllerViewModel::title() const
{
    // ViewModel 只读地使用领域模型，不把领域对象暴露给 QML
    return m_model->title();
}

void ApplicationControllerViewModel::refreshGreeting()
{
    ++m_refreshCount;
    m_greeting = tr("Hello from the ViewModel (#%1)").arg(m_refreshCount);

    LOG_KEY_INFO("ViewModel", m_greeting);

    // 状态变化后必须发信号，否则 QML 绑定不会刷新
    emit greetingChanged();
}
