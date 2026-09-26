#include "subnetresultpanel.h"

#include "ipgridpanel.h"
#include "lang.h"

#include <QAbstractItemView>
#include <QClipboard>
#include <QDesktopServices>
#include <QDialog>
#include <QGridLayout>
#include <QGuiApplication>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPalette>
#include <QProcess>
#include <QPushButton>
#include <QSettings>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTimer>
#include <QUrl>
#include <qt_windows.h>
#include <string>

namespace
{
const int kGridWidth = 480;

/// 结果表格列数：IP / MAC / 主机名 / 延迟 / 设备类型
const int kColumnCount = 5;

/// 导出时取自表格的列数（设备类型列不整体参与导出）
const int kExportSourceCount = 4;

/// 导出用列数：取自表格的列 + DHCP 服务器列 + 状态列（CSV 格式固定为 7 列，含网段）
const int kExportColumnCount = kExportSourceCount + 2;

/// 结果表格中「设备类型」列的列号
const int kDeviceTypeColumn = 4;

QColor dhcpColor() { return QColor(255, 138, 128); }
QColor cameraColor() { return QColor(123, 31, 162); }
QColor onlineColor() { return QColor(33, 150, 243); }
QColor offlineColor() { return QColor(76, 175, 80); }

QColor inactiveRowColor() { return QColor(245, 245, 245); }
QColor alternateRowColor() { return QColor(245, 248, 250); }

/// 合并「DHCP服务器 / 摄像头」两列后的设备类型文本；都不是时显示占位符
QString deviceTypeText(const DhcpServerInfo &info)
{
    QStringList types;
    if (info.isDhcpServer)
        types.append(Lang::get(QStringLiteral("ColDhcp")));
    if (info.isCamera)
        types.append(Lang::get(QStringLiteral("ColCamera")));

    return types.isEmpty() ? QStringLiteral("-") : types.join(QStringLiteral(" / "));
}
} // namespace

SubnetResultPanel::SubnetResultPanel(QWidget *parent)
    : QWidget(parent)
{
    m_grid = new QTableWidget(this);
    initTable();

    m_ipGrid = new IPGridPanel(this);
    m_ipGrid->setFixedWidth(kGridWidth);

    connect(m_grid, &QTableWidget::cellDoubleClicked, this,
            [this](int row, int) { showDetailDialog(row); });
    connect(m_ipGrid, &IPGridPanel::cellClicked, this,
            [this](int ipIndex) { jumpToRowByLastOctet(ipIndex + 1); });
    connect(m_ipGrid, &IPGridPanel::cellDoubleClicked, this, [this](int ipIndex) {
        jumpToRowByLastOctet(ipIndex + 1);
        if (m_grid->currentRow() >= 0)
            showDetailDialog(m_grid->currentRow());
    });

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_grid, 1);
    layout->addWidget(m_ipGrid);
}

void SubnetResultPanel::initTable()
{
    QFont gridFont = m_grid->font();
    gridFont.setPointSizeF(10.0);
    m_grid->setFont(gridFont);

    m_grid->setColumnCount(kColumnCount);
    m_grid->setHorizontalHeaderLabels(QStringList{
        Lang::get(QStringLiteral("ColIp")),
        Lang::get(QStringLiteral("ColMac")),
        Lang::get(QStringLiteral("ColHost")),
        Lang::get(QStringLiteral("ColPing")),
        Lang::get(QStringLiteral("ColDeviceType")),
    });

    m_grid->verticalHeader()->setVisible(false);
    m_grid->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_grid->setSelectionMode(QAbstractItemView::SingleSelection);
    m_grid->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_grid->setFrameShape(QFrame::NoFrame);
    m_grid->setShowGrid(true);
    m_grid->setWordWrap(false);

    QHeaderView *header = m_grid->horizontalHeader();
    header->setSectionsClickable(false);
    header->setHighlightSections(false);
    header->setFixedHeight(35);
    header->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    header->setSectionResizeMode(QHeaderView::Fixed);
    header->setStretchLastSection(false);
    QFont headerFont = gridFont;
    headerFont.setBold(true);
    header->setFont(headerFont);
    header->setStyleSheet(QStringLiteral(
        "QHeaderView::section {"
        "  background-color: rgb(0, 120, 215);"
        "  color: white;"
        "  font-weight: bold;"
        "  border: none;"
        "  padding-left: 4px;"
        "}"));

    const int widths[kColumnCount] = {135, 150, 140, 70, 185};
    for (int i = 0; i < kColumnCount; ++i)
        m_grid->setColumnWidth(i, widths[i]);

    QPalette palette = m_grid->palette();
    palette.setColor(QPalette::Base, Qt::white);
    palette.setColor(QPalette::Highlight, QColor(200, 220, 240));
    palette.setColor(QPalette::HighlightedText, Qt::black);
    m_grid->setPalette(palette);
}

void SubnetResultPanel::populateData(const QVector<DhcpServerInfo> &devices)
{
    m_rowInfos = devices;
    m_grid->clearContents();
    m_grid->setRowCount(devices.size());
    m_ipGrid->resetAllColors();

    for (int row = 0; row < devices.size(); ++row)
    {
        const DhcpServerInfo &info = devices.at(row);

        // 仅显示真实测得的延迟；在线但未测得（内存 -1）时显示占位符，避免出现 "-1"
        const QString pingText =
            (info.isActive && info.pingMs >= 0) ? QString::number(info.pingMs) : QStringLiteral("-");

        const QStringList texts{
            info.ipAddress,
            info.isActive ? info.macAddress : QStringLiteral("-"),
            info.isActive ? info.hostName : QStringLiteral("-"),
            pingText,
            deviceTypeText(info),
        };

        for (int column = 0; column < kColumnCount; ++column)
            m_grid->setItem(row, column, new QTableWidgetItem(texts.at(column)));

        // 摄像头用识别依据做提示，便于确认判定是否可靠
        if (info.isCamera && !info.cameraEvidence.isEmpty())
        {
            if (QTableWidgetItem *item = m_grid->item(row, kDeviceTypeColumn))
                item->setToolTip(info.cameraEvidence);
        }

        applyRowStyle(row, info);

        // 更新 IP 网格颜色
        const QStringList parts = info.ipAddress.split(QLatin1Char('.'));
        if (parts.size() == 4)
        {
            bool ok = false;
            const int ipLast = parts.at(3).toInt(&ok);
            if (ok)
            {
                const int index = ipLast - 1;
                if (index >= 0 && index < 255)
                {
                    if (info.isCamera)
                        m_ipGrid->setIpColor(index, cameraColor());
                    else if (info.isDhcpServer)
                        m_ipGrid->setIpColor(index, dhcpColor());
                    else if (info.isActive)
                        m_ipGrid->setIpColor(index, onlineColor());
                    else
                        m_ipGrid->setIpColor(index, offlineColor());
                }
            }
        }
    }
}

void SubnetResultPanel::applyRowStyle(int row, const DhcpServerInfo &info)
{
    const QFont gridFont = m_grid->font();

    // 交替行底色；非活跃行整行覆盖为灰色
    const QColor rowColor = info.isActive
                                ? ((row % 2 == 1) ? alternateRowColor() : QColor(Qt::white))
                                : inactiveRowColor();
    for (int column = 0; column < kColumnCount; ++column)
    {
        if (QTableWidgetItem *item = m_grid->item(row, column))
            item->setBackground(rowColor);
    }

    // 设备类型单元格强调：摄像头优先着色（与 IP 网格、详情弹窗一致）
    if (info.isDhcpServer || info.isCamera)
    {
        if (QTableWidgetItem *item = m_grid->item(row, kDeviceTypeColumn))
        {
            item->setForeground(info.isCamera ? cameraColor() : QColor(Qt::red));
            QFont font = gridFont;
            font.setBold(true);
            item->setFont(font);
        }
    }

    // IP 单元格用下划线强调特殊设备（摄像头优先于 DHCP 服务器）
    if (info.isCamera || info.isDhcpServer)
    {
        if (QTableWidgetItem *item = m_grid->item(row, 0))
        {
            item->setForeground(info.isCamera ? cameraColor() : QColor(0, 102, 204));
            QFont font = gridFont;
            font.setUnderline(true);
            item->setFont(font);
        }
    }
}

QList<QStringList> SubnetResultPanel::getRows() const
{
    QList<QStringList> rows;
    rows.reserve(m_grid->rowCount());

    for (int row = 0; row < m_grid->rowCount(); ++row)
    {
        QStringList values;
        values.reserve(kExportColumnCount);
        // 只取 IP / MAC / 主机名 / 延迟 前 4 列，设备类型列不整体参与导出
        for (int column = 0; column < kExportSourceCount; ++column)
        {
            const QTableWidgetItem *item = m_grid->item(row, column);
            values.append(item != nullptr ? item->text() : QString());
        }

        // DHCP 服务器标记按设备信息补回，保持 CSV 的 7 列格式不变
        const bool isDhcp = row < m_rowInfos.size() && m_rowInfos.at(row).isDhcpServer;
        values.append(isDhcp ? Lang::get(QStringLiteral("Yes")) : Lang::get(QStringLiteral("No")));

        // 表格中已不显示「状态」列，导出时按设备信息补回，保持 CSV 的 7 列格式不变
        const bool active = row < m_rowInfos.size() && m_rowInfos.at(row).isActive;
        values.append(active ? Lang::get(QStringLiteral("Online"))
                             : Lang::get(QStringLiteral("NoDevice")));

        rows.append(values);
    }

    return rows;
}

void SubnetResultPanel::refreshLanguage()
{
    m_grid->setHorizontalHeaderLabels(QStringList{
        Lang::get(QStringLiteral("ColIp")),
        Lang::get(QStringLiteral("ColMac")),
        Lang::get(QStringLiteral("ColHost")),
        Lang::get(QStringLiteral("ColPing")),
        Lang::get(QStringLiteral("ColDeviceType")),
    });

    // 刷新「设备类型」单元格中的文本（DHCP服务器 / 摄像头）
    for (int row = 0; row < m_grid->rowCount() && row < m_rowInfos.size(); ++row)
    {
        if (QTableWidgetItem *item = m_grid->item(row, kDeviceTypeColumn))
            item->setText(deviceTypeText(m_rowInfos.at(row)));
    }

    m_ipGrid->update();
}

void SubnetResultPanel::jumpToRowByLastOctet(int ipLast)
{
    const QString suffix = QStringLiteral(".") + QString::number(ipLast);

    for (int row = 0; row < m_grid->rowCount(); ++row)
    {
        const QTableWidgetItem *item = m_grid->item(row, 0);
        const QString ip = item != nullptr ? item->text() : QString();
        if (!ip.endsWith(suffix))
            continue;

        m_grid->clearSelection();
        m_grid->setCurrentCell(row, 0);
        m_grid->selectRow(row);
        m_grid->scrollToItem(m_grid->item(row, 0), QAbstractItemView::PositionAtTop);
        return;
    }
}

void SubnetResultPanel::showDetailDialog(int row)
{
    if (row < 0 || row >= m_grid->rowCount())
        return;

    const QTableWidgetItem *ipItem = m_grid->item(row, 0);
    const QTableWidgetItem *macItem = m_grid->item(row, 1);
    const QTableWidgetItem *hostItem = m_grid->item(row, 2);
    const QString ip = ipItem != nullptr ? ipItem->text() : QString();
    const QString mac = macItem != nullptr ? macItem->text() : QString();
    const QString host = hostItem != nullptr ? hostItem->text() : QString();

    bool isActive = false;
    bool isDhcp = false;
    bool isCamera = false;
    qint64 pingMs = -1;
    if (row < m_rowInfos.size())
    {
        isActive = m_rowInfos.at(row).isActive;
        isDhcp = m_rowInfos.at(row).isDhcpServer;
        isCamera = m_rowInfos.at(row).isCamera;
        pingMs = m_rowInfos.at(row).pingMs;
    }

    // 表格已把 DHCP/摄像头合并为「设备类型」列，这里按设备信息还原两项取值
    const QString dhcp =
        isDhcp ? Lang::get(QStringLiteral("Yes")) : Lang::get(QStringLiteral("No"));
    const QString camera =
        isCamera ? Lang::get(QStringLiteral("Yes")) : Lang::get(QStringLiteral("No"));

    const QColor statusColor = isCamera ? cameraColor()
                              : (isDhcp ? dhcpColor()
                                        : (isActive ? onlineColor() : QColor(158, 158, 158)));
    const QString pingText =
        (isActive && pingMs >= 0) ? QStringLiteral("%1 ms").arg(pingMs) : QStringLiteral("-");

    // 非模态显示：详情窗口不阻塞主窗口，可边看详情边继续操作
    QDialog *dialog = new QDialog(window());
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
    dialog->setWindowModality(Qt::NonModal);
    dialog->setWindowTitle(Lang::fmt(QStringLiteral("DeviceDetail"), ip));
    dialog->setStyleSheet(QStringLiteral("QDialog { background-color: white; }"));

    // 顶部状态色条
    QWidget *colorBar = new QWidget(dialog);
    colorBar->setGeometry(0, 0, 480, 6);
    colorBar->setAutoFillBackground(true);
    QPalette barPalette = colorBar->palette();
    barPalette.setColor(QPalette::Window, statusColor);
    colorBar->setPalette(barPalette);

    // 标题
    QLabel *lblTitle = new QLabel(ip, dialog);
    QFont titleFont = m_grid->font();
    titleFont.setPointSizeF(14.0);
    titleFont.setBold(true);
    lblTitle->setFont(titleFont);
    lblTitle->setStyleSheet(QStringLiteral("color: rgb(40, 40, 40);"));
    lblTitle->adjustSize();
    lblTitle->move(20, 20);

    // 状态标签
    QLabel *lblStatusTag = new QLabel(dialog);
    QStringList statusParts;
    if (isActive)
        statusParts.append(Lang::get(QStringLiteral("Online")));
    if (isCamera)
        statusParts.append(Lang::get(QStringLiteral("ColCamera")));
    if (isDhcp)
        statusParts.append(Lang::get(QStringLiteral("ColDhcp")));
    lblStatusTag->setText(statusParts.isEmpty()
                              ? Lang::get(QStringLiteral("NoDevice"))
                              : statusParts.join(QStringLiteral("  |  ")));
    QFont statusFont = m_grid->font();
    statusFont.setPointSizeF(9.0);
    statusFont.setBold(true);
    lblStatusTag->setFont(statusFont);
    lblStatusTag->setStyleSheet(QStringLiteral("color: white; background-color: rgb(%1, %2, %3);"
                                               " padding: 2px 6px;")
                                    .arg(statusColor.red())
                                    .arg(statusColor.green())
                                    .arg(statusColor.blue()));
    lblStatusTag->adjustSize();
    lblStatusTag->move(20, 48);

    struct Field
    {
        QString label;
        QString value;
        QColor color;
    };
    const Field fields[6] = {
        {Lang::get(QStringLiteral("FieldIp")), ip, QColor(40, 40, 40)},
        {Lang::get(QStringLiteral("FieldMac")), mac, QColor(40, 40, 40)},
        {Lang::get(QStringLiteral("FieldHost")), host, QColor(40, 40, 40)},
        {Lang::get(QStringLiteral("FieldPing")),
         pingText,
         pingText != QStringLiteral("-") ? QColor(46, 125, 50) : QColor(Qt::gray)},
        {Lang::get(QStringLiteral("ColDhcp")), dhcp, isDhcp ? QColor(Qt::red) : QColor(40, 40, 40)},
        {Lang::get(QStringLiteral("ColCamera")), camera, isCamera ? cameraColor() : QColor(40, 40, 40)},
    };

    const int infoTop = 86;
    const int rowHeight = 34;
    const int infoHeight = 6 * rowHeight;

    QWidget *infoTable = new QWidget(dialog);
    infoTable->setGeometry(20, infoTop, 440, infoHeight);
    infoTable->setStyleSheet(QStringLiteral("background-color: rgb(245, 247, 249);"));

    QGridLayout *infoLayout = new QGridLayout(infoTable);
    infoLayout->setContentsMargins(0, 0, 0, 0);
    infoLayout->setSpacing(0);

    QFont labelFont = m_grid->font();
    labelFont.setPointSizeF(9.5);
    QFont valueFont = labelFont;
    valueFont.setBold(true);

    const QString cellBorder = QStringLiteral("1px solid rgb(205, 210, 215)");
    for (int i = 0; i < 6; ++i)
    {
        const Field &field = fields[i];
        const QString topBorder = (i == 0) ? cellBorder : QStringLiteral("none");

        QLabel *label = new QLabel(field.label, infoTable);
        label->setFixedWidth(105);
        label->setFont(labelFont);
        label->setStyleSheet(QStringLiteral("color: rgb(100, 100, 100);"
                                            " background-color: rgb(245, 247, 249);"
                                            " border-top: %1; border-bottom: %2;"
                                            " border-left: %2; border-right: none;"
                                            " padding-left: 10px;")
                                 .arg(topBorder, cellBorder));
        infoLayout->addWidget(label, i, 0);

        QLineEdit *value = new QLineEdit(field.value, infoTable);
        value->setReadOnly(true);
        value->setFrame(false);
        value->setFont(valueFont);
        value->setStyleSheet(QStringLiteral("border-top: %1; border-bottom: %2;"
                                            " border-right: %2; border-left: none;"
                                            " background-color: rgb(245, 247, 249);"
                                            " padding-left: 8px; color: rgb(%3, %4, %5);")
                                 .arg(topBorder, cellBorder)
                                 .arg(field.color.red())
                                 .arg(field.color.green())
                                 .arg(field.color.blue()));
        infoLayout->addWidget(value, i, 1);
    }
    infoLayout->setColumnStretch(1, 1);

    const int y = infoTop + infoHeight + 16;

    const AppLanguage language = Lang::current();
    QString copyDetailsText;
    QString copiedText;
    switch (language)
    {
    case AppLanguage::English:
        copyDetailsText = QStringLiteral("Copy details");
        copiedText = QStringLiteral("Copied");
        break;
    case AppLanguage::TraditionalChinese:
        copyDetailsText = QStringLiteral("複製資訊");
        copiedText = QStringLiteral("已複製");
        break;
    case AppLanguage::TraditionalChineseHk:
        copyDetailsText = QStringLiteral("複製資料");
        copiedText = QStringLiteral("已複製");
        break;
    default:
        copyDetailsText = QStringLiteral("复制详情");
        copiedText = QStringLiteral("已复制");
        break;
    }

    const int btnW = 120;
    const int btnH = 35;
    const int gap = 10;

    QFont buttonFont = m_grid->font();
    buttonFont.setPointSizeF(9.0);
    QFont boldButtonFont = buttonFont;
    boldButtonFont.setBold(true);

    QPushButton *btnCopy = new QPushButton(copyDetailsText, dialog);
    btnCopy->setFixedSize(btnW, btnH);
    btnCopy->setCursor(Qt::PointingHandCursor);
    btnCopy->setFont(buttonFont);

    QPushButton *btnPing = new QPushButton(QStringLiteral("Ping"), dialog);
    btnPing->setFixedSize(btnW, btnH);
    btnPing->setCursor(Qt::PointingHandCursor);
    btnPing->setFont(boldButtonFont);

    QPushButton *btnWeb = new QPushButton(Lang::get(QStringLiteral("AccessAdmin")), dialog);
    btnWeb->setFixedSize(btnW, btnH);
    btnWeb->setCursor(Qt::PointingHandCursor);
    btnWeb->setFont(boldButtonFont);

    QPushButton *btnIe = new QPushButton(Lang::get(QStringLiteral("IeAccess")), dialog);
    btnIe->setFixedSize(btnW, btnH);
    btnIe->setCursor(Qt::PointingHandCursor);
    btnIe->setFont(boldButtonFont);

    QPushButton *btnClose = new QPushButton(Lang::get(QStringLiteral("Close")), dialog);
    btnClose->setFixedSize(btnW, btnH);
    btnClose->setCursor(Qt::PointingHandCursor);
    btnClose->setFont(boldButtonFont);

    connect(btnCopy, &QPushButton::clicked, dialog,
            [fields, btnCopy, copyDetailsText, copiedText]() {
                QStringList lines;
                for (const Field &field : fields)
                    lines.append(QStringLiteral("%1: %2").arg(field.label, field.value));

                if (QClipboard *clipboard = QGuiApplication::clipboard())
                    clipboard->setText(lines.join(QStringLiteral("\r\n")));

                btnCopy->setText(copiedText);
                QTimer::singleShot(1400, btnCopy, [btnCopy, copyDetailsText]() {
                    btnCopy->setText(copyDetailsText);
                });
            });

    connect(btnPing, &QPushButton::clicked, dialog, [ip]() {
        // 本程序是 GUI 程序、自身没有控制台。Qt 启动子进程时会检测到这一点并给
        // 子进程附加 CREATE_NO_WINDOW，导致 cmd 控制台窗口被“隐藏创建”（进程在跑，
        // 却看不到窗口）。这里绕开 Qt 的标志处理，直接调用 Win32 API 并显式指定
        // CREATE_NEW_CONSOLE，确保 ping 窗口正常弹出。
        std::wstring command = QStringLiteral("cmd.exe /k ping %1 -t").arg(ip).toStdWString();
        command.push_back(L'\0'); // CreateProcessW 需要可写、以 null 结尾的命令行缓冲

        STARTUPINFOW startupInfo;
        ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);
        PROCESS_INFORMATION processInfo;
        ZeroMemory(&processInfo, sizeof(processInfo));

        if (CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE, CREATE_NEW_CONSOLE,
                           nullptr, nullptr, &startupInfo, &processInfo))
        {
            CloseHandle(processInfo.hThread);
            CloseHandle(processInfo.hProcess);
        }
    });

    connect(btnWeb, &QPushButton::clicked, dialog,
            [ip]() { QDesktopServices::openUrl(QUrl(QStringLiteral("http://") + ip)); });

    connect(btnIe, &QPushButton::clicked, dialog, [dialog, ip]() {
        QSettings ieReg(QStringLiteral("HKEY_CLASSES_ROOT\\InternetExplorer.Application"),
                        QSettings::NativeFormat);
        if (ieReg.allKeys().isEmpty() && ieReg.childGroups().isEmpty())
        {
            QMessageBox::information(dialog, Lang::get(QStringLiteral("Tip")),
                                     Lang::get(QStringLiteral("IeNotRegistered")));
            return;
        }

        if (!QProcess::startDetached(QStringLiteral("iexplore.exe"),
                                     QStringList{QStringLiteral("http://") + ip}))
        {
            QMessageBox::information(
                dialog, Lang::get(QStringLiteral("Tip")),
                Lang::fmt(QStringLiteral("IeLaunchFailed"), QStringLiteral("iexplore.exe")));
        }
    });

    connect(btnClose, &QPushButton::clicked, dialog, &QDialog::reject);

    const QList<QPushButton *> allButtons{btnCopy, btnPing, btnWeb, btnIe, btnClose};
    QList<QPushButton *> visibleButtons;
    for (QPushButton *button : allButtons)
    {
        const bool visible = (button == btnWeb || button == btnIe) ? (isDhcp || isCamera) : true;
        button->setVisible(visible);
        if (visible)
            visibleButtons.append(button);
    }

    const int columns = qMin(3, visibleButtons.size());
    const int rows = (visibleButtons.size() + columns - 1) / columns;
    dialog->setFixedSize(480, y + rows * btnH + (rows - 1) * gap + 20);

    for (int i = 0; i < visibleButtons.size(); ++i)
    {
        const int r = i / columns;
        const int c = i % columns;
        const int rowCount = qMin(columns, visibleButtons.size() - r * columns);
        const int rowWidth = rowCount * btnW + (rowCount - 1) * gap;
        const int rowStartX = (480 - rowWidth) / 2;
        visibleButtons.at(i)->setGeometry(rowStartX + c * (btnW + gap), y + r * (btnH + gap), btnW,
                                          btnH);
    }

    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}
