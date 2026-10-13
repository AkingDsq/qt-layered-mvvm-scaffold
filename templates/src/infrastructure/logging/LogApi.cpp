#include "LogApi.h"

#include "LoggerService.h"

#include <QMutex>
#include <QMutexLocker>

namespace
{
    QMutex g_loggerMutex;
    std::shared_ptr<LoggerService> g_activeLogger;

    QString safeString(const char *text)
    {
        return text ? QString::fromUtf8(text) : QString();
    }

    std::shared_ptr<LoggerService> currentLogger()
    {
        QMutexLocker locker(&g_loggerMutex);
        return g_activeLogger;
    }
} // namespace

namespace LogApi
{

    void setActiveLogger(const std::shared_ptr<LoggerService> &logger)
    {
        QMutexLocker locker(&g_loggerMutex);
        g_activeLogger = logger;
    }

    std::shared_ptr<LoggerService> activeLogger()
    {
        return currentLogger();
    }

    void clear()
    {
        auto logger = currentLogger();
        if (!logger)
        {
            return;
        }
        logger->clear();
    }

    void info(const QString &category, const QString &message)
    {
        auto logger = currentLogger();
        if (!logger)
        {
            return;
        }
        logger->append(LoggerService::LogLevel::Info, category, message);
    }

    void warning(const QString &category, const QString &message)
    {
        auto logger = currentLogger();
        if (!logger)
        {
            return;
        }
        logger->append(LoggerService::LogLevel::Warning, category, message);
    }

    void error(const QString &category, const QString &message)
    {
        auto logger = currentLogger();
        if (!logger)
        {
            return;
        }
        logger->append(LoggerService::LogLevel::Error, category, message);
    }

    void info(const char *category, const char *message)
    {
        info(safeString(category), safeString(message));
    }

    void info(const char *category, const QString &message)
    {
        info(safeString(category), message);
    }

    void warning(const char *category, const char *message)
    {
        warning(safeString(category), safeString(message));
    }

    void warning(const char *category, const QString &message)
    {
        warning(safeString(category), message);
    }

    void error(const char *category, const char *message)
    {
        error(safeString(category), safeString(message));
    }

    void error(const char *category, const QString &message)
    {
        error(safeString(category), message);
    }

} // namespace LogApi
