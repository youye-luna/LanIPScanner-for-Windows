#pragma once

#include <QDialog>

#include "appsettings.h"

class QComboBox;
class QGroupBox;
class QLabel;
class QPushButton;
class QRadioButton;
class QSpinBox;
class QWidget;

/// 设置窗口（界面语言、扫描线程数、历史数据保存配置）。
class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

private:
    void initRangeSelection();
    void updateRangeVisibility();
    void applySaveConfig();

    AppSettings m_settings;

    QLabel *m_lblLanguage = nullptr;
    QComboBox *m_comboLanguage = nullptr;
    QLabel *m_lblThreads = nullptr;
    QSpinBox *m_numThreads = nullptr;
    QLabel *m_lblHint = nullptr;

    QGroupBox *m_groupMethod = nullptr;
    QRadioButton *m_radioByTime = nullptr;
    QRadioButton *m_radioByCount = nullptr;

    QGroupBox *m_groupRange = nullptr;
    QWidget *m_panelTimeRange = nullptr;
    QWidget *m_panelCountRange = nullptr;

    QRadioButton *m_radioDays14 = nullptr;
    QRadioButton *m_radioDaysHalf = nullptr;
    QRadioButton *m_radioDaysMonth = nullptr;
    QRadioButton *m_radioDaysYear = nullptr;
    QRadioButton *m_radioNever = nullptr;
    QRadioButton *m_radioCustom = nullptr;
    QSpinBox *m_numCustomDays = nullptr;

    QRadioButton *m_radioCount30 = nullptr;
    QRadioButton *m_radioCount60 = nullptr;
    QRadioButton *m_radioCount90 = nullptr;
    QRadioButton *m_radioCount100 = nullptr;

    QPushButton *m_btnSaveSettings = nullptr;
    QPushButton *m_btnAbout = nullptr;
    QPushButton *m_btnOk = nullptr;
    QPushButton *m_btnCancel = nullptr;
};
