#ifndef LOGGERSERVICE_H
#define LOGGERSERVICE_H

#include <QMetaType>
#include <QMutex>
#include <QObject>
#include <QString>
#include <QVector>
#include <QtGlobal>

struct LogEntry
{
    QString timestamp; // 时间戳
    QString level;     // 日志级别
    QString category;  // 日志类别
    QString message;   // 日志消息
};

Q_DECLARE_METATYPE(LogEntry)

// 基础设施服务：线程亲和的日志实现。
// 约定：非拥有者线程通过 Qt::QueuedConnection 投递，保证 m_entries 只在拥有者线程被改写。
class LoggerService : public QObject
{
    Q_OBJECT
public:
    enum class LogLevel
    {
        Info,
        Warning,
        Error
    };
    Q_ENUM(LogLevel)

    explicit LoggerService(QObject *parent = nullptr);

    void initialize(const QString &logPath);                                      // 初始化日志系统
    void append(LogLevel level, const QString &category, const QString &message); // 添加日志条目
    QVector<LogEntry> entriesSnapshot() const;                                    // 获取日志条目的快照
    int entryCount() const;                                                       // 当前日志条目数量
    bool tryGetEntry(int index, LogEntry &entry) const;                           // 按索引读取单条日志
    QString formattedEntry(const LogEntry &entry) const;                          // 格式化单条日志
    QString allLogsText() const;                                                  // 按统一格式返回全部日志文本
    void clear();

signals:
    void logAdded(const LogEntry &entry);
    void logsCleared();

private:
    LogEntry buildEntry(LogLevel level, const QString &category, const QString &message) const; // 生成日志条目
    QString levelToString(LogLevel level) const;                                                // 将日志级别转换为字符串
    QString formatForFile(const LogEntry &entry) const;                                         // 格式化日志条目以写入文件
    void appendOnOwnerThread(const LogEntry &entry);                                            // 在拥有者线程上添加日志条目
    void rotateFileIfNeededLocked();                                                            // 在持有锁的情况下检查并执行日志文件轮转

private:
    mutable QMutex m_mutex;                  // 保护日志条目和文件操作的互斥锁
    QVector<LogEntry> m_entries;             // 日志条目列表
    QString m_logDirPath;                    // 日志目录路径
    QString m_logFilePath;                   // 当前日志文件路径
    bool m_initialized = false;              // 是否已初始化
    int m_maxEntries = 2000;                 // 最大日志条目数
    qint64 m_maxFileBytes = 2 * 1024 * 1024; // 最大日志文件大小（2MB）
};

#endif // LOGGERSERVICE_H
