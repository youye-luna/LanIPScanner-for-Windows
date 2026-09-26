#include "settingsdialog.h"

#include <QColor>
#include <QComboBox>
#include <QFont>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QString>
#include <QWidget>

#include "aboutdialog.h"
#include "lang.h"
#include "scanhistory.h"

namespace {
const int kClientWidth = 460;
const int kClientHeight = 500;

const QString kLangSimpleChinese = QStringLiteral("简体中文");
const QString kLangTraditionalTw = QStringLiteral("繁體中文（中国台湾）");
const QString kLangTraditionalHk = QStringLiteral("繁體中文（中国香港/澳门）");
const QString kLangEnglish = QStringLiteral("English");

/// 枚举 → 下拉显示文本
QString languageDisplay(AppLanguage language)
{
    switch (language) {
    case AppLanguage::English:
        return kLangEnglish;
    case AppLanguage::TraditionalChinese:
        return kLangTraditionalTw;
    case AppLanguage::TraditionalChineseHk:
        return kLangTraditionalHk;
    default:
        return kLangSimpleChinese;
    }
}

/// 下拉显示文本 → 枚举
AppLanguage languageParse(const QString &text)
{
    if (text == kLangEnglish)
        return AppLanguage::English;
    if (text == kLangTraditionalTw)
        return AppLanguage::TraditionalChinese;
    if (text == kLangTraditionalHk)
        return AppLanguage::TraditionalChineseHk;
    return AppLanguage::Chinese;
}

QRadioButton *makeRadio(QWidget *parent, const QString &text, int x, int y)
{
    QRadioButton *radio = new QRadioButton(text, parent);
    radio->setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    radio->move(x, y);
    radio->adjustSize();
    return radio;
}

QPushButton *makeButton(QWidget *parent, const QString &text, int x, int y, int w, int h)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setGeometry(x, y, w, h);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}
} // namespace

SettingsDialog::SettingsDialog(QWidget *parent)
    : QDialog(parent)
{
    m_settings = AppSettings::load();

    setWindowTitle(Lang::get(QStringLiteral("SettingsTitle")));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(kClientWidth, kClientHeight);
    setStyleSheet(QStringLiteral("QDialog { background-color: white; }"));
    setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    setWindowIcon(QIcon(QStringLiteral(":/app.ico")));

    // ------------------------------------------------------------ 界面语言
    m_lblLanguage = new QLabel(Lang::get(QStringLiteral("LanguageLabel")), this);
    m_lblLanguage->move(20, 24);
    m_lblLanguage->adjustSize();

    m_comboLanguage = new QComboBox(this);
    m_comboLanguage->setGeometry(150, 20, 200, 25);
    // 中文系列排在英文前面
    m_comboLanguage->addItem(kLangSimpleChinese);
    m_comboLanguage->addItem(kLangTraditionalTw);
    m_comboLanguage->addItem(kLangTraditionalHk);
    m_comboLanguage->addItem(kLangEnglish);
    m_comboLanguage->setCurrentIndex(
        m_comboLanguage->findText(languageDisplay(m_settings.language)));

    // ------------------------------------------------------------ 扫描线程数
    m_lblThreads = new QLabel(Lang::get(QStringLiteral("ThreadsLabel")), this);
    m_lblThreads->move(20, 64);
    m_lblThreads->adjustSize();

    m_numThreads = new QSpinBox(this);
    m_numThreads->setGeometry(150, 60, 80, 25);
    m_numThreads->setRange(1, 100);
    m_numThreads->setValue(qBound(1, m_settings.scanThreads, 100));
    m_numThreads->setAlignment(Qt::AlignCenter);

    m_lblHint = new QLabel(Lang::get(QStringLiteral("ThreadsHint")), this);
    m_lblHint->setGeometry(20, 98, 420, 34);
    m_lblHint->setWordWrap(true);
    m_lblHint->setFont(QFont(QStringLiteral("Microsoft YaHei"), 8));
    {
        QPalette palette = m_lblHint->palette();
        palette.setColor(QPalette::WindowText, QColor(150, 150, 150));
        m_lblHint->setPalette(palette);
    }

    // ------------------------------------------------------------ 数据保存方式
    m_groupMethod = new QGroupBox(Lang::get(QStringLiteral("SaveMethodGroup")), this);
    m_groupMethod->setGeometry(15, 140, 430, 68);
    {
        QFont font = m_groupMethod->font();
        font.setFamily(QStringLiteral("Microsoft YaHei"));
        font.setPointSize(9);
        font.setBold(true);
        m_groupMethod->setFont(font);
    }

    m_radioByTime = makeRadio(m_groupMethod, Lang::get(QStringLiteral("SaveByTime")), 24, 32);
    m_radioByCount = makeRadio(m_groupMethod, Lang::get(QStringLiteral("SaveByCount")), 190, 32);

    // ------------------------------------------------------------ 保存范围
    m_groupRange = new QGroupBox(Lang::get(QStringLiteral("SaveRangeGroup")), this);
    m_groupRange->setGeometry(15, 218, 430, 124);
    {
        QFont font = m_groupRange->font();
        font.setFamily(QStringLiteral("Microsoft YaHei"));
        font.setPointSize(9);
        font.setBold(true);
        m_groupRange->setFont(font);
    }

    // 时间范围选项（独立容器，避免与数量选项互相排斥）
    m_panelTimeRange = new QWidget(m_groupRange);
    m_panelTimeRange->setGeometry(6, 22, 418, 96);

    m_radioDays14 = makeRadio(m_panelTimeRange, Lang::get(QStringLiteral("Range14Days")), 18, 10);
    m_radioDaysHalf = makeRadio(m_panelTimeRange, Lang::get(QStringLiteral("RangeHalfMonth")), 230, 10);
    m_radioDaysMonth = makeRadio(m_panelTimeRange, Lang::get(QStringLiteral("RangeOneMonth")), 18, 38);
    m_radioDaysYear = makeRadio(m_panelTimeRange, Lang::get(QStringLiteral("RangeOneYear")), 230, 38);
    m_radioNever = makeRadio(m_panelTimeRange, Lang::get(QStringLiteral("RangeNever")), 18, 66);
    m_radioCustom = makeRadio(m_panelTimeRange, Lang::get(QStringLiteral("RangeCustom")), 230, 66);

    m_numCustomDays = new QSpinBox(m_panelTimeRange);
    m_numCustomDays->setGeometry(330, 66, 72, 25);
    m_numCustomDays->setRange(1, 3650);
    m_numCustomDays->setValue(30);
    m_numCustomDays->setAlignment(Qt::AlignCenter);

    // 数量范围选项（独立容器）
    m_panelCountRange = new QWidget(m_groupRange);
    m_panelCountRange->setGeometry(6, 22, 418, 72);

    m_radioCount30 = makeRadio(m_panelCountRange, Lang::get(QStringLiteral("Range30")), 18, 20);
    m_radioCount60 = makeRadio(m_panelCountRange, Lang::get(QStringLiteral("Range60")), 230, 20);
    m_radioCount90 = makeRadio(m_panelCountRange, Lang::get(QStringLiteral("Range90")), 18, 48);
    m_radioCount100 = makeRadio(m_panelCountRange, Lang::get(QStringLiteral("Range100")), 230, 48);

    /** 根据配置初始化选中项 */
    initRangeSelection();
    m_radioByTime->setChecked(m_settings.historySaveMode != HistorySaveMode::ByCount);
    m_radioByCount->setChecked(m_settings.historySaveMode == HistorySaveMode::ByCount);
    updateRangeVisibility();
    // 自定义天数输入框仅在“自定义天数”选中时可用
    m_numCustomDays->setEnabled(m_radioCustom->isChecked());

    connect(m_radioByTime, &QRadioButton::toggled, this, &SettingsDialog::updateRangeVisibility);
    connect(m_radioByCount, &QRadioButton::toggled, this, &SettingsDialog::updateRangeVisibility);
    connect(m_radioCustom, &QRadioButton::toggled, m_numCustomDays, &QWidget::setEnabled);

    // ------------------------------------------------------------ 按钮
    m_btnSaveSettings = makeButton(this, Lang::get(QStringLiteral("SaveSettings")), 15, 352, 110, 32);
    connect(m_btnSaveSettings, &QPushButton::clicked, this, [this]() {
        applySaveConfig();
        m_settings.save();
        ScanHistoryStore::prune();
        QMessageBox::information(this, Lang::get(QStringLiteral("Success")),
                                 Lang::get(QStringLiteral("SaveConfigSuccess")));
    });

    m_btnAbout = makeButton(this, Lang::get(QStringLiteral("About")), 135, 352, 85, 32);
    connect(m_btnAbout, &QPushButton::clicked, this, [this]() {
        AboutDialog about(this);
        about.exec();
    });

    m_btnOk = makeButton(this, Lang::get(QStringLiteral("Ok")), 270, 452, 85, 32);
    connect(m_btnOk, &QPushButton::clicked, this, [this]() {
        m_settings.language = languageParse(m_comboLanguage->currentText());
        m_settings.scanThreads = m_numThreads->value();
        applySaveConfig();
        m_settings.save();
        ScanHistoryStore::prune();
        accept();
    });

    m_btnCancel = makeButton(this, Lang::get(QStringLiteral("Cancel")), 365, 452, 85, 32);
    connect(m_btnCancel, &QPushButton::clicked, this, &QDialog::reject);

    m_btnOk->setDefault(true);
}

void SettingsDialog::initRangeSelection()
{
    switch (m_settings.historySaveDays) {
    case 14:
        m_radioDays14->setChecked(true);
        break;
    case 15:
        m_radioDaysHalf->setChecked(true);
        break;
    case 30:
        m_radioDaysMonth->setChecked(true);
        break;
    case 365:
        m_radioDaysYear->setChecked(true);
        break;
    case 0:
        m_radioNever->setChecked(true);
        break;
    default:
        // 自定义天数
        m_radioCustom->setChecked(true);
        m_numCustomDays->setValue(qBound(1, m_settings.historySaveDays, 3650));
        break;
    }

    switch (m_settings.historySaveMaxRecords) {
    case 30:
        m_radioCount30->setChecked(true);
        break;
    case 60:
        m_radioCount60->setChecked(true);
        break;
    case 90:
        m_radioCount90->setChecked(true);
        break;
    default:
        m_radioCount100->setChecked(true);
        break;
    }
}

void SettingsDialog::updateRangeVisibility()
{
    const bool byTime = m_radioByTime->isChecked();
    m_panelTimeRange->setVisible(byTime);
    m_panelCountRange->setVisible(!byTime);
}

void SettingsDialog::applySaveConfig()
{
    m_settings.historySaveMode =
        m_radioByTime->isChecked() ? HistorySaveMode::ByTime : HistorySaveMode::ByCount;

    if (m_settings.historySaveMode == HistorySaveMode::ByTime) {
        if (m_radioDays14->isChecked())
            m_settings.historySaveDays = 14;
        else if (m_radioDaysHalf->isChecked())
            m_settings.historySaveDays = 15;
        else if (m_radioDaysMonth->isChecked())
            m_settings.historySaveDays = 30;
        else if (m_radioDaysYear->isChecked())
            m_settings.historySaveDays = 365;
        else if (m_radioNever->isChecked())
            m_settings.historySaveDays = 0;
        else
            m_settings.historySaveDays = m_numCustomDays->value(); // 自定义天数
    } else {
        if (m_radioCount30->isChecked())
            m_settings.historySaveMaxRecords = 30;
        else if (m_radioCount60->isChecked())
            m_settings.historySaveMaxRecords = 60;
        else if (m_radioCount90->isChecked())
            m_settings.historySaveMaxRecords = 90;
        else
            m_settings.historySaveMaxRecords = 100;
    }
}
