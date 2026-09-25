#include "mainwindow.h"

#include "appsettings.h"
#include "historydialog.h"
#include "ipaddressedit.h"
#include "lang.h"
#include "netutils.h"
#include "settingsdialog.h"
#include "subnetresultpanel.h"

#include <QCloseEvent>
#include <QDateTime>
#include <QDialog>
#include <QFile>
#include <QFileDialog>
#include <QFont>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMap>
#include <QMessageBox>
#include <QPalette>
#include <QProgressBar>
#include <QPushButton>
#include <QResizeEvent>
#include <QShowEvent>
#include <QStatusBar>
#include <QTabWidget>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWinTaskbarButton>
#include <QWinTaskbarProgress>
#include <algorithm>

namespace
{
const char *kTooManySubnetsPrefix = "TOO_MANY_SUBNETS:";

/// C# SaveFileDialog 的 "显示名|通配符|显示名|通配符" 过滤器转换为 Qt 格式
QString toQtFilter(const QString &winFilter)
{
    const QStringList parts = winFilter.split(QLatin1Char('|'));
    QStringList display;
    for (int i = 0; i < parts.size(); i += 2)
        display.append(parts.at(i));
    return display.join(QStringLiteral(";;"));
}

/// CSV 字段转义（与 C# EscapeCsvField 一致）
QString escapeCsvField(const QString &field)
{
    if (field.contains(QLatin1Char(',')) || field.contains(QLatin1Char('"'))
        || field.contains(QLatin1Char('\n')))
    {
        QString escaped = field;
        escaped.replace(QLatin1Char('"'), QStringLiteral("\"\""));
        return QLatin1Char('"') + escaped + QLatin1Char('"');
    }
    return field;
}

void applyWindowColor(QWidget *widget, const QColor &color)
{
    widget->setAutoFillBackground(true);
    QPalette pal = widget->palette();
    pal.setColor(QPalette::Window, color);
    widget->setPalette(pal);
}

/// 扫描完成汇总窗口：对勾图标 + 标题 + 分组统计卡片（替代纯文本堆叠的 QMessageBox）
void showScanSummaryDialog(QWidget *parent, int totalIps, int online, int noDevice,
                           int router, int camera)
{
    struct StatRow
    {
        const char *key;
        int value;
        const char *color;
    };
    const StatRow rows[] = {
        {"Online", online, "rgb(33,150,243)"},      // 在线
        {"NoDevice", noDevice, "rgb(130,135,140)"}, // 无设备
        {"ColDhcp", router, "rgb(229,57,53)"},      // DHCP服务器
        {"ColCamera", camera, "rgb(123,31,162)"},   // 摄像头
    };

    QDialog dialog(parent);
    dialog.setWindowTitle(Lang::get(QStringLiteral("ScanCompletedStatus")));
    dialog.setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    dialog.setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    dialog.setFixedWidth(380);
    dialog.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    dialog.setStyleSheet(QStringLiteral("QDialog { background: white; }"));

    // ---- 顶部：绿色对勾 + 标题 + 扫描总数 ----
    QLabel *checkIcon = new QLabel(QStringLiteral("\u2713"), &dialog);
    checkIcon->setFixedSize(38, 38);
    checkIcon->setAlignment(Qt::AlignCenter);
    QFont checkFont(QStringLiteral("Segoe UI Symbol"), 16);
    checkFont.setBold(true);
    checkIcon->setFont(checkFont);
    checkIcon->setStyleSheet(
        QStringLiteral("background: rgb(76,175,80); color: white; border-radius: 19px;"));

    QLabel *title = new QLabel(Lang::get(QStringLiteral("ScanCompletedStatus")), &dialog);
    QFont titleFont(QStringLiteral("Microsoft YaHei"), 12);
    titleFont.setBold(true);
    title->setFont(titleFont);
    title->setStyleSheet(QStringLiteral("color: rgb(45,50,55);"));

    QLabel *subtitle =
        new QLabel(Lang::fmt(QStringLiteral("ScanSummaryTotalIps"), totalIps), &dialog);
    subtitle->setStyleSheet(QStringLiteral("color: rgb(140,145,150);"));

    QVBoxLayout *titleLayout = new QVBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);
    titleLayout->addWidget(title);
    titleLayout->addWidget(subtitle);

    QHBoxLayout *headerLayout = new QHBoxLayout;
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(14);
    headerLayout->addWidget(checkIcon);
    headerLayout->addLayout(titleLayout, 1);

    // ---- 中部：统计卡片（左名称 / 右数值，数值按类别着色） ----
    QWidget *card = new QWidget(&dialog);
    card->setObjectName(QStringLiteral("summaryCard"));
    card->setStyleSheet(QStringLiteral(
        "QWidget#summaryCard { background: rgb(246,248,250); border-radius: 8px; }"));

    QGridLayout *grid = new QGridLayout(card);
    grid->setContentsMargins(18, 14, 18, 14);
    grid->setHorizontalSpacing(12);
    grid->setVerticalSpacing(9);

    for (int i = 0; i < 4; ++i)
    {
        QLabel *name = new QLabel(Lang::get(QLatin1String(rows[i].key)), card);
        name->setStyleSheet(QStringLiteral("color: rgb(105,110,115);"));

        QLabel *value = new QLabel(QString::number(rows[i].value), card);
        QFont valueFont(QStringLiteral("Microsoft YaHei"), 11);
        valueFont.setBold(true);
        value->setFont(valueFont);
        value->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        value->setStyleSheet(QStringLiteral("color: %1;").arg(QLatin1String(rows[i].color)));

        grid->addWidget(name, i, 0);
        grid->addWidget(value, i, 1);
    }
    grid->setColumnStretch(1, 1);

    // ---- 底部：确定按钮 ----
    QPushButton *okButton = new QPushButton(Lang::get(QStringLiteral("Ok")), &dialog);
    okButton->setFixedSize(96, 30);
    okButton->setCursor(Qt::PointingHandCursor);
    okButton->setDefault(true);
    okButton->setStyleSheet(QStringLiteral(
        "QPushButton { background: rgb(33,150,243); color: white; border: none;"
        " border-radius: 4px; }"
        "QPushButton:hover { background: rgb(30,136,229); }"
        "QPushButton:pressed { background: rgb(25,118,210); }"));

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(okButton);

    QVBoxLayout *dialogLayout = new QVBoxLayout(&dialog);
    dialogLayout->setContentsMargins(24, 20, 24, 18);
    dialogLayout->setSpacing(16);
    dialogLayout->addLayout(headerLayout);
    dialogLayout->addWidget(card);
    dialogLayout->addLayout(buttonLayout);

    QObject::connect(okButton, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.adjustSize();
    dialog.exec();
}
} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QWidget(parent)
{
    // 加载设置（语言、扫描线程数）
    const AppSettings settings = AppSettings::load();
    Lang::setCurrent(settings.language);

    buildUi();

    m_scanner = new Scanner(this);
    m_scanner->setMaxParallelism(settings.scanThreads);

    connect(m_scanner, &Scanner::scanProgress, this, &MainWindow::onScanProgress);
    connect(m_scanner, &Scanner::scanCompleted, this, &MainWindow::onScanCompleted);
    connect(m_scanner, &Scanner::scanError, this, &MainWindow::onScanError);
    connect(m_scanner, &Scanner::scanFinished, this, &MainWindow::onScanFinished);

    // 应用界面语言
    applyLanguage();

    // 启动时按保存配置清理过期历史记录
    ScanHistoryStore::prune();

    // 初始布局按钮位置
    layoutSearchPanel();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildUi()
{
    setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    resize(1200, 800);
    setMinimumSize(1040, 700);
    applyWindowColor(this, Qt::white);

    // ---------------- 搜索面板 ----------------
    m_panelSearch = new QWidget(this);
    m_panelSearch->setFixedHeight(112);
    applyWindowColor(m_panelSearch, QColor(240, 244, 247));

    QFont labelFont(QStringLiteral("Microsoft YaHei"), 9);
    QFont boldTitleFont(QStringLiteral("Microsoft YaHei"), 10);
    boldTitleFont.setBold(true);
    QFont buttonFont(QStringLiteral("Microsoft YaHei"), 9);

    m_labelTitle = new QLabel(m_panelSearch);
    m_labelTitle->setFont(boldTitleFont);
    m_labelTitle->setStyleSheet(QStringLiteral("color: rgb(50,50,50);"));

    m_labelStartIp = new QLabel(m_panelSearch);
    m_labelStartIp->setFont(labelFont);
    m_labelStartIp->setStyleSheet(QStringLiteral("color: rgb(60,60,60);"));

    m_ipStart = new IpAddressEdit(m_panelSearch);
    m_ipStart->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));

    m_labelTo = new QLabel(m_panelSearch);
    m_labelTo->setFont(labelFont);
    m_labelTo->setStyleSheet(QStringLiteral("color: rgb(120,120,120);"));

    m_labelEndIp = new QLabel(m_panelSearch);
    m_labelEndIp->setFont(labelFont);
    m_labelEndIp->setStyleSheet(QStringLiteral("color: rgb(60,60,60);"));

    m_ipEnd = new IpAddressEdit(m_panelSearch);
    m_ipEnd->setFont(QFont(QStringLiteral("Microsoft YaHei UI"), 9));

    // 默认起始/结束 IP 取本机 IP 前三段
    QStringList parts = NetUtils::localIpv4Address().split(QLatin1Char('.'));
    if (parts.size() != 4)
        parts = QStringList{QStringLiteral("192"), QStringLiteral("168"),
                            QStringLiteral("1"), QStringLiteral("1")};
    m_ipStart->setAddress(QStringLiteral("%1.%2.%3.1").arg(parts[0], parts[1], parts[2]));
    m_ipEnd->setAddress(QStringLiteral("%1.%2.%3.255").arg(parts[0], parts[1], parts[2]));

    m_flowButtons = new QWidget(m_panelSearch);
    QHBoxLayout *flowLayout = new QHBoxLayout(m_flowButtons);
    flowLayout->setContentsMargins(3, 0, 3, 0);
    flowLayout->setSpacing(6);

    m_buttonScan = new QPushButton(m_flowButtons);
    m_buttonStop = new QPushButton(m_flowButtons);
    m_buttonClear = new QPushButton(m_flowButtons);
    m_buttonExport = new QPushButton(m_flowButtons);
    QPushButton *flowButtonList[] = {m_buttonScan, m_buttonStop, m_buttonClear, m_buttonExport};
    for (QPushButton *button : flowButtonList)
    {
        button->setFixedSize(100, 30);
        button->setFont(buttonFont);
        button->setCursor(Qt::PointingHandCursor);
        flowLayout->addWidget(button);
    }
    flowLayout->addStretch(1);
    m_buttonStop->setEnabled(false);

    m_buttonSettings = new QPushButton(m_panelSearch);
    m_buttonHistory = new QPushButton(m_panelSearch);
    QPushButton *panelButtonList[] = {m_buttonSettings, m_buttonHistory};
    for (QPushButton *button : panelButtonList)
    {
        button->setFixedSize(100, 30);
        button->setFont(buttonFont);
        button->setCursor(Qt::PointingHandCursor);
    }

    // ---------------- 进度（显示在 Windows 任务栏图标上） ----------------
    m_taskbarButton = new QWinTaskbarButton(this);
    m_taskbarProgress = m_taskbarButton->progress();

    // ---------------- 扫描进度弹窗（弹出显示真实百分比） ----------------
    m_progressDialog = new QDialog(this);
    m_progressDialog->setWindowModality(Qt::ApplicationModal);
    m_progressDialog->setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    m_progressDialog->setFixedWidth(400);

    m_progressRangeLabel = new QLabel(m_progressDialog);
    m_progressRangeLabel->setFont(labelFont);
    m_progressRangeLabel->setStyleSheet(QStringLiteral("color: rgb(60,60,60);"));

    m_progressBar = new QProgressBar(m_progressDialog);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->setAlignment(Qt::AlignCenter);
    m_progressBar->setFixedHeight(24);
    m_progressBar->setStyleSheet(QStringLiteral(
        "QProgressBar { border: 1px solid rgb(190,195,200); border-radius: 4px;"
        " background: rgb(240,244,247); text-align: center; }"
        "QProgressBar::chunk { background: rgb(33,150,243); border-radius: 3px; }"));

    m_progressHintLabel = new QLabel(m_progressDialog);
    m_progressHintLabel->setFont(labelFont);
    m_progressHintLabel->setWordWrap(true);
    m_progressHintLabel->setStyleSheet(QStringLiteral("color: rgb(120,120,120);"));

    m_buttonStopInDialog = new QPushButton(m_progressDialog);
    m_buttonStopInDialog->setFixedSize(100, 30);
    m_buttonStopInDialog->setFont(buttonFont);
    m_buttonStopInDialog->setCursor(Qt::PointingHandCursor);

    QHBoxLayout *progressButtonLayout = new QHBoxLayout;
    progressButtonLayout->setContentsMargins(0, 0, 0, 0);
    progressButtonLayout->addStretch(1);
    progressButtonLayout->addWidget(m_buttonStopInDialog);
    progressButtonLayout->addStretch(1);

    QVBoxLayout *progressLayout = new QVBoxLayout(m_progressDialog);
    progressLayout->setContentsMargins(22, 18, 22, 16);
    progressLayout->setSpacing(12);
    progressLayout->addWidget(m_progressRangeLabel);
    progressLayout->addWidget(m_progressBar);
    progressLayout->addWidget(m_progressHintLabel);
    progressLayout->addLayout(progressButtonLayout);

    connect(m_buttonStopInDialog, &QPushButton::clicked, this, &MainWindow::onStopClicked);

    // ---------------- 结果标签页 ----------------
    m_tabControlResults = new QTabWidget(this);
    m_tabControlResults->setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    m_tabControlResults->setStyleSheet(
        QStringLiteral("QTabBar::tab { padding: 4px 12px; }"));
    m_tabControlResults->setDocumentMode(true);

    // ---------------- 底部状态栏 ----------------
    m_panelBottom = new QWidget(this);
    m_panelBottom->setFixedHeight(30);
    applyWindowColor(m_panelBottom, QColor(240, 244, 247));

    QStatusBar *statusBar = new QStatusBar(m_panelBottom);
    statusBar->setSizeGripEnabled(false);
    statusBar->setStyleSheet(QStringLiteral(
        "QStatusBar { background: transparent; } QStatusBar::item { border: none; }"));

    m_statusLabel = new QLabel(statusBar);
    m_statusLabel->setFont(labelFont);
    m_statusCount = new QLabel(statusBar);
    m_statusCount->setFont(labelFont);
    statusBar->addWidget(m_statusLabel, 1);
    statusBar->addWidget(m_statusCount, 0);

    QHBoxLayout *bottomLayout = new QHBoxLayout(m_panelBottom);
    // 左右留白，避免状态栏文字顶到窗口边缘（视觉上被裁切）
    bottomLayout->setContentsMargins(8, 0, 14, 0);
    bottomLayout->addWidget(statusBar);

    // ---------------- 窗体布局 ----------------
    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);
    rootLayout->addWidget(m_panelSearch);
    rootLayout->addWidget(m_tabControlResults, 1);
    rootLayout->addWidget(m_panelBottom);

    // ---------------- 信号连接 ----------------
    connect(m_buttonScan, &QPushButton::clicked, this, &MainWindow::onScanClicked);
    connect(m_buttonStop, &QPushButton::clicked, this, &MainWindow::onStopClicked);
    connect(m_buttonClear, &QPushButton::clicked, this, &MainWindow::onClearClicked);
    connect(m_buttonExport, &QPushButton::clicked, this, &MainWindow::onExportClicked);
    connect(m_buttonSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(m_buttonHistory, &QPushButton::clicked, this, &MainWindow::onHistoryClicked);
}

void MainWindow::applyLanguage()
{
    setWindowTitle(Lang::get(QStringLiteral("FormTitle")));
    m_labelTitle->setText(Lang::get(QStringLiteral("ScanRangeTitle")));
    m_labelStartIp->setText(Lang::get(QStringLiteral("StartIp")));
    m_labelTo->setText(Lang::get(QStringLiteral("To")));
    m_labelEndIp->setText(Lang::get(QStringLiteral("EndIp")));
    m_buttonScan->setText(Lang::get(QStringLiteral("StartScan")));
    m_buttonStop->setText(Lang::get(QStringLiteral("StopScan")));
    m_buttonClear->setText(Lang::get(QStringLiteral("ClearResults")));
    m_buttonExport->setText(Lang::get(QStringLiteral("ExportResults")));
    m_buttonSettings->setText(Lang::get(QStringLiteral("Settings")));
    m_buttonHistory->setText(Lang::get(QStringLiteral("History")));
    m_statusLabel->setText(Lang::get(QStringLiteral("Ready")));
    m_statusCount->setText(Lang::get(QStringLiteral("StatusCountInit")));

    // 扫描进度弹窗
    m_progressDialog->setWindowTitle(Lang::get(QStringLiteral("ScanProgressTitle")));
    m_progressHintLabel->setText(Lang::get(QStringLiteral("ScanningInProgress")));
    m_buttonStopInDialog->setText(Lang::get(QStringLiteral("StopScan")));

    // 标签自适应宽度（Label.AutoSize）
    const QLabel *labels[] = {m_labelTitle, m_labelStartIp, m_labelTo, m_labelEndIp};
    for (const QLabel *label : labels)
        const_cast<QLabel *>(label)->adjustSize();

    // 刷新所有已打开的结果面板（表格列头 + IP分布图标题/图例）
    for (int i = 0; i < m_tabControlResults->count(); ++i)
    {
        QWidget *page = m_tabControlResults->widget(i);
        QString subnet = page->property("subnet").toString();
        if (subnet.isEmpty())
            subnet = m_tabControlResults->tabText(i);
        m_tabControlResults->setTabText(i, Lang::fmt(QStringLiteral("SubnetTab"), subnet));
        if (SubnetResultPanel *panel = qobject_cast<SubnetResultPanel *>(page))
            panel->refreshLanguage();
    }
}

void MainWindow::layoutSearchPanel()
{
    if (m_panelSearch == nullptr || m_flowButtons == nullptr)
        return;

    // 按钮行放在 IP 输入框下方
    const int buttonsY = 34 + 28 + 8;
    const int paddingRight = 20;
    const int panelWidth = m_panelSearch->width();
    const int settingsWidth = m_buttonSettings->width();
    const int historyWidth = m_buttonHistory->width();

    m_buttonHistory->move(panelWidth - paddingRight - settingsWidth - historyWidth - 8, buttonsY);
    m_buttonSettings->move(panelWidth - paddingRight - settingsWidth, buttonsY);

    const int flowWidth = panelWidth - 20 - paddingRight - settingsWidth - historyWidth - 10 - 8;
    m_flowButtons->setGeometry(20, buttonsY, qMax(flowWidth, 0), 36);

    // IP 输入框与标签绝对定位（对应设计器的 Location/Size）
    m_labelTitle->move(20, 8);
    m_labelStartIp->move(20, 38);
    m_ipStart->setGeometry(80, 34, 248, 28);
    m_labelTo->move(335, 38);
    m_labelEndIp->move(365, 38);
    m_ipEnd->setGeometry(425, 34, 248, 28);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    layoutSearchPanel();
}

void MainWindow::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    // 任务栏进度需要窗口句柄，首次显示后绑定一次
    if (m_taskbarButton != nullptr && m_taskbarButton->window() != windowHandle())
        m_taskbarButton->setWindow(windowHandle());
}

void MainWindow::setScanningUiEnabled(bool scanning)
{
    m_buttonScan->setEnabled(!scanning);
    m_buttonStop->setEnabled(scanning);
    m_buttonClear->setEnabled(!scanning);
    m_buttonExport->setEnabled(!scanning);
    m_buttonSettings->setEnabled(!scanning);
    m_buttonHistory->setEnabled(!scanning);
}

void MainWindow::onScanClicked()
{
    if (m_isScanning)
    {
        QMessageBox::information(this, Lang::get(QStringLiteral("Tip")),
                                 Lang::get(QStringLiteral("ScanningInProgress")));
        return;
    }

    const QString startIp = m_ipStart->address();
    const QString endIp = m_ipEnd->address();

    if (startIp.isEmpty() || endIp.isEmpty())
    {
        QMessageBox::critical(this, Lang::get(QStringLiteral("Error")),
                              Lang::get(QStringLiteral("InputIpRequired")));
        return;
    }

    // 限制只能扫描内网地址
    if (!NetUtils::isPrivateIp(startIp) || !NetUtils::isPrivateIp(endIp))
    {
        QMessageBox::critical(this, Lang::get(QStringLiteral("Error")),
                              Lang::get(QStringLiteral("PrivateIpOnly")));
        return;
    }

    // 清除之前的标签页
    m_tabControlResults->clear();

    // 记录本次扫描范围（用于自动保存历史）
    m_currentStartIp = startIp;
    m_currentEndIp = endIp;

    // 更新UI状态
    m_isScanning = true;
    setScanningUiEnabled(true);
    m_taskbarProgress->setRange(0, 100);
    m_taskbarProgress->setValue(0);
    m_taskbarProgress->setVisible(true);
    m_statusLabel->setText(Lang::fmt(QStringLiteral("ScanningRange"), startIp, endIp));
    m_statusCount->setText(Lang::get(QStringLiteral("StatusCountScanning")));

    // 弹出进度弹窗（百分比来自 Scanner 的真实进度信号）
    showProgressDialog();

    QString errorMessage;
    if (!m_scanner->startIpRangeScan(startIp, endIp, &errorMessage))
    {
        const QString prefix = QString::fromLatin1(kTooManySubnetsPrefix);
        if (errorMessage.startsWith(prefix))
        {
            showTooManySubnetsMessage(errorMessage.mid(prefix.size()).toInt());
        }
        else
        {
            QMessageBox::critical(this, Lang::get(QStringLiteral("Error")),
                                  Lang::fmt(QStringLiteral("ScanErrorStatus"), errorMessage));
        }

        // 与 C# 的 finally 一致：恢复界面状态
        m_isScanning = false;
        setScanningUiEnabled(false);
        m_taskbarProgress->setVisible(false);
        closeProgressDialog();
    }
}

void MainWindow::onStopClicked()
{
    if (m_isScanning)
    {
        m_scanner->stopScan();
        m_statusLabel->setText(Lang::get(QStringLiteral("ScanStopped")));
    }
}

void MainWindow::onClearClicked()
{
    m_tabControlResults->clear();
    m_taskbarProgress->setVisible(false);
    m_statusCount->setText(Lang::get(QStringLiteral("StatusCountInit")));
    m_statusLabel->setText(Lang::get(QStringLiteral("Ready")));
}

void MainWindow::onExportClicked()
{
    if (m_tabControlResults->count() == 0)
    {
        QMessageBox::information(this, Lang::get(QStringLiteral("Tip")),
                                 Lang::get(QStringLiteral("NoDataToExport")));
        return;
    }

    const QString filter = toQtFilter(Lang::get(QStringLiteral("ExportFilter")));
    const QString defaultName =
        QStringLiteral("%1_%2.csv")
            .arg(Lang::get(QStringLiteral("ExportFileName")),
                 QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss")));

    const QString filePath = QFileDialog::getSaveFileName(
        this, Lang::get(QStringLiteral("ExportResults")), defaultName, filter);
    if (filePath.isEmpty())
        return;

    QString errorMessage;
    if (exportToCsv(filePath, &errorMessage))
    {
        QMessageBox::information(this, Lang::get(QStringLiteral("Success")),
                                 Lang::fmt(QStringLiteral("ExportSuccess"), filePath));
    }
    else
    {
        QMessageBox::critical(this, Lang::get(QStringLiteral("Error")),
                              Lang::fmt(QStringLiteral("ExportFailed"), errorMessage));
    }
}

bool MainWindow::exportToCsv(const QString &filePath, QString *errorMessage)
{
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
    {
        if (errorMessage != nullptr)
            *errorMessage = file.errorString();
        return false;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");
    out.setGenerateByteOrderMark(true);
    out << Lang::get(QStringLiteral("CsvHeader")) << "\r\n";

    for (int i = 0; i < m_tabControlResults->count(); ++i)
    {
        QWidget *page = m_tabControlResults->widget(i);
        SubnetResultPanel *panel = qobject_cast<SubnetResultPanel *>(page);
        if (panel == nullptr)
            continue;

        QString subnet = page->property("subnet").toString();
        if (subnet.isEmpty())
            subnet = m_tabControlResults->tabText(i);

        const QList<QStringList> rows = panel->getRows();
        for (const QStringList &row : rows)
        {
            QStringList fields;
            fields.reserve(6);
            for (int column = 0; column < 6; ++column)
                fields.append(escapeCsvField(column < row.size() ? row.at(column) : QString()));
            out << fields.join(QLatin1Char(',')) << "\r\n";
        }
    }

    out.flush();
    file.close();
    return true;
}

void MainWindow::showProgressDialog()
{
    m_progressRangeLabel->setText(
        Lang::fmt(QStringLiteral("ScanningRange"), m_currentStartIp, m_currentEndIp));
    m_progressBar->setValue(0);
    m_progressDialog->show();
}

void MainWindow::closeProgressDialog()
{
    if (m_progressDialog != nullptr && m_progressDialog->isVisible())
        m_progressDialog->hide();
}

void MainWindow::onScanProgress(int progress)
{
    const int value = qMin(progress, 100);
    m_taskbarProgress->setValue(value);
    m_progressBar->setValue(value);
    m_statusLabel->setText(Lang::fmt(QStringLiteral("ScanProgressPercent"), progress));
}

void MainWindow::onScanCompleted(const QVector<DhcpServerInfo> &results)
{
    m_taskbarProgress->setValue(100);
    closeProgressDialog();
    m_statusLabel->setText(Lang::get(QStringLiteral("OrganizingResults")));

    // 自动保存扫描历史
    saveScanHistory(results);

    populateResultTabs(results);

    int totalOnline = 0;
    int totalRouter = 0;
    int totalCamera = 0;
    for (const DhcpServerInfo &info : results)
    {
        if (info.isActive)
            ++totalOnline;
        if (info.isDhcpServer)
            ++totalRouter;
        if (info.isCamera)
            ++totalCamera;
    }
    const int totalNoDevice = results.size() - totalOnline;

    showScanSummaryDialog(this, results.size(), totalOnline, totalNoDevice, totalRouter,
                          totalCamera);
}

void MainWindow::saveScanHistory(const QVector<DhcpServerInfo> &results)
{
    ScanHistoryRecord record;
    record.scanTime = QDateTime::currentDateTime();
    record.startIp = m_currentStartIp;
    record.endIp = m_currentEndIp;
    record.devices.reserve(results.size());
    for (const DhcpServerInfo &info : results)
        record.devices.append(ScanHistoryDevice::from(info));
    ScanHistoryStore::save(&record);
}

void MainWindow::populateResultTabs(const QVector<DhcpServerInfo> &results)
{
    // 按网段分组（前三段），网段名按字符串升序
    QMap<QString, QVector<DhcpServerInfo>> groups;
    for (const DhcpServerInfo &info : results)
    {
        const QStringList parts = info.ipAddress.split(QLatin1Char('.'));
        const QString key = parts.size() >= 3 ? parts.mid(0, 3).join(QLatin1Char('.'))
                                              : info.ipAddress;
        groups[key].append(info);
    }

    m_tabControlResults->clear();

    for (auto it = groups.constBegin(); it != groups.constEnd(); ++it)
    {
        QVector<DhcpServerInfo> subnetResults = it.value();
        std::sort(subnetResults.begin(), subnetResults.end(),
                  [](const DhcpServerInfo &left, const DhcpServerInfo &right) {
                      return NetUtils::ipToLong(left.ipAddress)
                             < NetUtils::ipToLong(right.ipAddress);
                  });

        SubnetResultPanel *panel = new SubnetResultPanel(m_tabControlResults);
        panel->populateData(subnetResults);
        panel->setProperty("subnet", it.key());
        m_tabControlResults->addTab(panel, Lang::fmt(QStringLiteral("SubnetTab"), it.key()));
    }

    // 统计总数
    int totalOnline = 0;
    int totalRouter = 0;
    for (const DhcpServerInfo &info : results)
    {
        if (info.isActive)
            ++totalOnline;
        if (info.isDhcpServer)
            ++totalRouter;
    }
    const int totalNoDevice = results.size() - totalOnline;

    m_statusCount->setText(Lang::fmt(QStringLiteral("StatusCountDone"), totalOnline,
                                     totalNoDevice, totalRouter));
    m_statusLabel->setText(Lang::get(QStringLiteral("ScanCompletedStatus")));
}

void MainWindow::displayHistoryRecord(const ScanHistoryRecord &record)
{
    QVector<DhcpServerInfo> results;
    results.reserve(record.devices.size());
    for (const ScanHistoryDevice &device : record.devices)
        results.append(device.toServerInfo());

    populateResultTabs(results);
    m_statusLabel->setText(
        Lang::fmt(QStringLiteral("HistoryLoaded"), record.startIp, record.endIp));
}

void MainWindow::onScanError(const QString &message)
{
    closeProgressDialog();
    m_statusLabel->setText(Lang::fmt(QStringLiteral("ScanErrorStatus"), message));
    QMessageBox::critical(this, Lang::get(QStringLiteral("Error")),
                          Lang::fmt(QStringLiteral("ScanErrorDialog"), message));
}

void MainWindow::onScanFinished()
{
    m_isScanning = false;
    setScanningUiEnabled(false);
    // 扫描结束后隐藏任务栏进度与进度弹窗
    m_taskbarProgress->setVisible(false);
    closeProgressDialog();
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        // 重新加载已保存的设置并应用
        const AppSettings settings = AppSettings::load();
        Lang::setCurrent(settings.language);
        m_scanner->setMaxParallelism(settings.scanThreads);
        applyLanguage();
    }
}

void MainWindow::onHistoryClicked()
{
    HistoryDialog dialog(this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    const ScanHistoryRecord record = dialog.selectedRecord();
    if (record.filePath.isEmpty() && record.devices.isEmpty())
        return;

    displayHistoryRecord(record);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_isScanning)
    {
        const QMessageBox::StandardButton result = QMessageBox::question(
            this, Lang::get(QStringLiteral("Confirm")),
            Lang::get(QStringLiteral("ExitWhileScanning")),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (result != QMessageBox::Yes)
        {
            event->ignore();
            return;
        }

        m_scanner->stopScan();
    }

    event->accept();
}

void MainWindow::showTooManySubnetsMessage(int count)
{
    QDialog dialog(this);
    dialog.setWindowTitle(Lang::get(QStringLiteral("Tip")));
    dialog.setFixedSize(360, 180);
    dialog.setModal(true);
    applyWindowColor(&dialog, Qt::white);

    QLabel *labelMain = new QLabel(Lang::get(QStringLiteral("MaxSubnets")), &dialog);
    QFont mainFont(QStringLiteral("Microsoft YaHei"), 12);
    mainFont.setBold(true);
    labelMain->setFont(mainFont);
    labelMain->setStyleSheet(QStringLiteral("color: rgb(50,50,50);"));
    labelMain->setAlignment(Qt::AlignCenter);
    labelMain->setGeometry(20, 25, 310, 35);

    QLabel *labelSub = new QLabel(Lang::fmt(QStringLiteral("TooManySubnetsSub"), count), &dialog);
    labelSub->setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    labelSub->setStyleSheet(QStringLiteral("color: rgb(160,160,160);"));
    labelSub->setAlignment(Qt::AlignCenter);
    labelSub->setWordWrap(true);
    labelSub->setGeometry(20, 60, 310, 45);

    QPushButton *buttonOk = new QPushButton(Lang::get(QStringLiteral("Ok")), &dialog);
    buttonOk->setGeometry(135, 105, 80, 30);
    buttonOk->setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    buttonOk->setCursor(Qt::PointingHandCursor);
    connect(buttonOk, &QPushButton::clicked, &dialog, &QDialog::accept);

    dialog.adjustSize();
    dialog.setFixedSize(360, 180);
    const QPoint center = geometry().center();
    dialog.move(center.x() - dialog.width() / 2, center.y() - dialog.height() / 2);

    dialog.exec();
}
