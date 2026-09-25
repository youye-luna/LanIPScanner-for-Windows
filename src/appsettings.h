#pragma once

#include "lang.h"

#include <QString>

/// 扫描历史保存方式
enum class HistorySaveMode
{
    ByTime = 0, ///< 按时间保存（保留最近 N 天）
    ByCount = 1 ///< 按数量保存（保留最近 N 条）
};

/// 应用设置（持久化到程序目录下的 settings.json）
struct AppSettings
{
    AppLanguage language = AppLanguage::Chinese;
    int scanThreads = 30;
    HistorySaveMode historySaveMode = HistorySaveMode::ByCount;
    int historySaveDays = 30;
    int historySaveMaxRecords = 100;

    /// 从配置文件加载设置，首次运行时自动检测系统语言并保存
    static AppSettings load();

    /// 根据系统区域设置自动检测语言
    static AppLanguage detectSystemLanguage();

    /// 保存设置到配置文件
    void save() const;

    /// 配置文件所在目录（程序所在目录）
    static QString configDir();

    /// 配置文件完整路径
    static QString filePath();
};
