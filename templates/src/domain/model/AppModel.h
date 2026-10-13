#ifndef APPMODEL_H
#define APPMODEL_H

#include <QObject>
#include <QString>

// 领域模型（MVVM 的 Model）——中性占位，请替换成你自己的领域概念。
//
// 替换时保持这三条约定，其余随意：
//   1. 只依赖 Qt Core 与 infrastructure 的门面，不引用任何界面类型和 ViewModel；
//   2. 公开继承 QObject 并把 parent 交给基类，让 Qt 对象树管理生命周期；
//   3. 只暴露只读查询，不把可变内部状态交给调用方。
class AppModel : public QObject
{
    Q_OBJECT

public:
    explicit AppModel(QObject *parent = nullptr);
    ~AppModel() override;

    // 占位状态：替换成真正的领域数据
    QString title() const;

private:
    QString m_title;
};

#endif // APPMODEL_H
