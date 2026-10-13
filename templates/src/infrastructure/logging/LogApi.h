#ifndef LOGAPI_H
#define LOGAPI_H

#include <QString>
#include <memory>

class LoggerService;

// 日志门面：让上层只依赖这一组函数与宏，而不依赖 LoggerService 的具体实例。
namespace LogApi
{

    void setActiveLogger(const std::shared_ptr<LoggerService> &logger);
    std::shared_ptr<LoggerService> activeLogger();

    void clear();

    void info(const QString &category, const QString &message);
    void warning(const QString &category, const QString &message);
    void error(const QString &category, const QString &message);

    void info(const char *category, const char *message);
    void info(const char *category, const QString &message);
    void warning(const char *category, const char *message);
    void warning(const char *category, const QString &message);
    void error(const char *category, const char *message);
    void error(const char *category, const QString &message);

} // namespace LogApi

#define LOG_KEY_INFO(category, message) ::LogApi::info((category), (message))
#define LOG_KEY_WARNING(category, message) ::LogApi::warning((category), (message))
#define LOG_KEY_ERROR(category, message) ::LogApi::error((category), (message))

#endif // LOGAPI_H
