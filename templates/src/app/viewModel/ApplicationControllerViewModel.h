#ifndef APPLICATIONCONTROLLERVIEWMODEL_H
#define APPLICATIONCONTROLLERVIEWMODEL_H

#include <QObject>
#include <QString>

class AppModel;

// ViewModel：向 QML 暴露只读状态与命令，持有并编排领域模型。
// 规则：不得包含任何 QML 类型；不得直接操作界面。
class ApplicationControllerViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString greeting READ greeting NOTIFY greetingChanged)
    // title 取自领域模型且在构造后不再变化，所以用 CONSTANT。
    // 若你的模型会变，改成 NOTIFY xxxChanged 并在变化时发信号。
    Q_PROPERTY(QString title READ title CONSTANT)

public:
    explicit ApplicationControllerViewModel(QObject *parent = nullptr);
    ~ApplicationControllerViewModel() override;

    QString greeting() const;
    QString title() const;

public slots:
    void refreshGreeting();

signals:
    void greetingChanged();

private:
    AppModel *m_model = nullptr;
    QString m_greeting;
    int m_refreshCount = 0;
};

#endif // APPLICATIONCONTROLLERVIEWMODEL_H
