// appsettings.cpp —— 应用设置的读写（对应 C# AppSettings.cs）
//
// 配置文件名、结构、键名（PascalCase）与 C# 版保持一致，便于沿用已有 settings.json。

#include "appsettings.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLocale>

#include <algorithm>

namespace
{
int clampInt(int value, int minimum, int maximum)
{
    return std::max(minimum, std::min(maximum, value));
}
} // namespace

QString AppSettings::configDir()
{
    return QCoreApplication::applicationDirPath();
}

QString AppSettings::filePath()
{
    return QDir(configDir()).filePath(QStringLiteral("settings.json"));
}

AppLanguage AppSettings::detectSystemLanguage()
{
    const QLocale locale = QLocale::system();
    if (locale.language() != QLocale::Chinese)
        return AppLanguage::English;

    switch (locale.script())
    {
    case QLocale::SimplifiedHanScript:
        return AppLanguage::Chinese;
    case QLocale::TraditionalHanScript:
        // 香港/澳门使用繁体，但用词与台湾不同（如 網絡/資料）
        if (locale.country() == QLocale::HongKong || locale.country() == QLocale::Macau)
            return AppLanguage::TraditionalChineseHk;
        return AppLanguage::TraditionalChinese;
    default:
        break;
    }

    // 脚本未指定时按地区判断
    const QLocale::Country country = locale.country();
    if (country == QLocale::China || country == QLocale::Singapore)
        return AppLanguage::Chinese;
    if (country == QLocale::HongKong || country == QLocale::Macau)
        return AppLanguage::TraditionalChineseHk;
    return AppLanguage::TraditionalChinese;
}

AppSettings AppSettings::load()
{
    const QString path = filePath();
    if (QFileInfo::exists(path))
    {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly))
        {
            const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
            file.close();
            if (document.isObject())
            {
                const QJsonObject object = document.object();
                AppSettings settings;
                settings.language = static_cast<AppLanguage>(
                    clampInt(object.value(QStringLiteral("Language")).toInt(static_cast<int>(AppLanguage::Chinese)), 0, 3));
                settings.scanThreads = clampInt(
                    object.value(QStringLiteral("ScanThreads")).toInt(30), 1, 100);
                settings.historySaveMode = static_cast<HistorySaveMode>(
                    clampInt(object.value(QStringLiteral("HistorySaveMode")).toInt(static_cast<int>(HistorySaveMode::ByCount)), 0, 1));
                settings.historySaveDays = clampInt(
                    object.value(QStringLiteral("HistorySaveDays")).toInt(30), 0, 3650);
                settings.historySaveMaxRecords = clampInt(
                    object.value(QStringLiteral("HistorySaveMaxRecords")).toInt(100), 1, 10000);
                return settings;
            }
        }
    }

    // 首次运行：根据系统语言自动设置，然后保存配置文件
    AppSettings settings;
    settings.language = detectSystemLanguage();
    settings.save();
    return settings;
}

void AppSettings::save() const
{
    const QString dir = configDir();
    if (!QDir().mkpath(dir))
        return;

    QJsonObject object;
    object.insert(QStringLiteral("Language"), static_cast<int>(language));
    object.insert(QStringLiteral("ScanThreads"), scanThreads);
    object.insert(QStringLiteral("HistorySaveMode"), static_cast<int>(historySaveMode));
    object.insert(QStringLiteral("HistorySaveDays"), historySaveDays);
    object.insert(QStringLiteral("HistorySaveMaxRecords"), historySaveMaxRecords);

    QFile file(filePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return;
    file.write(QJsonDocument(object).toJson(QJsonDocument::Indented));
    file.close();
}
