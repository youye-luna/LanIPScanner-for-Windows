#pragma once

#include <QString>
#include <QStringList>

/// 界面语言
enum class AppLanguage
{
    Chinese = 0,             ///< 简体中文
    English = 1,             ///< English
    TraditionalChinese = 2,  ///< 繁体中文（台湾）
    TraditionalChineseHk = 3 ///< 繁体中文（香港/澳门）
};

/// 多语言文本管理（简体中文 / English / 繁体中文·台湾 / 繁体中文·香港）
namespace Lang
{
AppLanguage current();
void setCurrent(AppLanguage lang);

/// 根据键获取当前语言的文本，缺失时返回键本身
QString get(const QString &key);

/// 使用已格式化好的参数替换 {0}..{n}
QString fmtArgs(const QString &key, const QStringList &args);

inline QString argToString(const QString &value) { return value; }
inline QString argToString(const char *value) { return QString::fromUtf8(value); }
inline QString argToString(int value) { return QString::number(value); }
inline QString argToString(long long value) { return QString::number(value); }
inline QString argToString(double value) { return QString::number(value); }

/// 取当前语言的文本并替换 {0}..{n}
template <typename... Args>
inline QString fmt(const QString &key, Args... args)
{
    return fmtArgs(key, QStringList{argToString(args)...});
}
} // namespace Lang
