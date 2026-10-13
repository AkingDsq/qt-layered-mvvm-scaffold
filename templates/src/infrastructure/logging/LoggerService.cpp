#include "LoggerService.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMetaObject>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QStringList>
#include <QTextStream>
#include <QThread>

LoggerService::LoggerService(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<LogEntry>("LogEntry");
}

void LoggerService::initialize(const QString &logPath)
{
    QMutexLocker locker(&m_mutex);
    if (m_initialized)
    {
        return;
    }

    m_logDirPath = QDir(logPath).filePath("logs");
    QDir().mkpath(m_logDirPath);
    m_logFilePath = QDir(m_logDirPath).filePath("app.log");
    m_initialized = true;
}

void LoggerService::append(LogLevel level, const QString &category, const QString &message)
{
    if (message.trimmed().isEmpty())
    {
        return;
    }

    if (!m_initialized)
    {
        qDebug() << "LoggerService is not initialized. Call initialize() before appending logs.";
        return;
    }

    const LogEntry entry = buildEntry(level, category, message);

    // 如果当前线程就是 LoggerService 所在线程，直接添加日志；否则通过 invokeMethod 在 LoggerService 线程上添加日志
    if (QThread::currentThread() == thread())
    {
        appendOnOwnerThread(entry);
        return;
    }

    QMetaObject::invokeMethod(this, [this, entry]()
                              { appendOnOwnerThread(entry); }, Qt::QueuedConnection);
}

QVector<LogEntry> LoggerService::entriesSnapshot() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries;
}

int LoggerService::entryCount() const
{
    QMutexLocker locker(&m_mutex);
    return m_entries.size();
}

bool LoggerService::tryGetEntry(int index, LogEntry &entry) const
{
    QMutexLocker locker(&m_mutex);
    if (index < 0 || index >= m_entries.size())
    {
        return false;
    }

    entry = m_entries.at(index);
    return true;
}

QString LoggerService::formattedEntry(const LogEntry &entry) const
{
    return QString("[%1] [%2] [%3] %4")
        .arg(entry.timestamp, entry.level, entry.category, entry.message);
}

QString LoggerService::allLogsText() const
{
    QVector<LogEntry> entries;
    {
        QMutexLocker locker(&m_mutex);
        entries = m_entries;
    }

    QStringList lines;
    lines.reserve(entries.size());
    for (const LogEntry &entry : entries)
    {
        lines << formattedEntry(entry);
    }

    return lines.join("\n");
}

void LoggerService::clear()
{
    auto clearInternal = [this]()
    {
        {
            QMutexLocker locker(&m_mutex);
            m_entries.clear();

            if (!m_logFilePath.isEmpty())
            {
                QFile file(m_logFilePath);
                if (file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
                {
                    file.close();
                }
            }
        }

        emit logsCleared();
    };

    if (QThread::currentThread() == thread())
    {
        clearInternal();
        return;
    }

    QMetaObject::invokeMethod(this, clearInternal, Qt::QueuedConnection);
}

LogEntry LoggerService::buildEntry(LogLevel level, const QString &category, const QString &message) const
{
    LogEntry entry;
    entry.timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    entry.level = levelToString(level);
    entry.category = category.trimmed().isEmpty() ? QStringLiteral("General") : category.trimmed();
    entry.message = message;
    return entry;
}

QString LoggerService::levelToString(LogLevel level) const
{
    switch (level)
    {
    case LogLevel::Info:
        return QStringLiteral("INFO");
    case LogLevel::Warning:
        return QStringLiteral("WARNING");
    case LogLevel::Error:
        return QStringLiteral("ERROR");
    default:
        return QStringLiteral("INFO");
    }
}

QString LoggerService::formatForFile(const LogEntry &entry) const
{
    return QString("[%1] [%2] [%3] %4")
        .arg(entry.timestamp, entry.level, entry.category, entry.message);
}

void LoggerService::appendOnOwnerThread(const LogEntry &entry)
{
    {
        QMutexLocker locker(&m_mutex);

        if (m_entries.size() >= m_maxEntries)
        {
            const int removeCount = m_entries.size() - m_maxEntries + 1;
            m_entries.remove(0, removeCount);
        }

        m_entries.push_back(entry);

        if (!m_logFilePath.isEmpty())
        {
            rotateFileIfNeededLocked();

            QFile file(m_logFilePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text))
            {
                QTextStream stream(&file);
                stream << formatForFile(entry) << Qt::endl;
            }

            rotateFileIfNeededLocked();
        }
    }

    emit logAdded(entry);
}

void LoggerService::rotateFileIfNeededLocked()
{
    if (m_logFilePath.isEmpty())
    {
        return;
    }

    QFileInfo info(m_logFilePath);
    if (!info.exists() || info.size() < m_maxFileBytes)
    {
        return;
    }

    const QString rotatedPath = m_logFilePath + ".1";
    if (QFile::exists(rotatedPath))
    {
        QFile::remove(rotatedPath);
    }
    QFile::rename(m_logFilePath, rotatedPath);
}
