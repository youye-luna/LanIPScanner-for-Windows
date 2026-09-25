#include "historydialog.h"

#include <QAbstractItemView>
#include <QColor>
#include <QFont>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPalette>
#include <QPushButton>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>
#include <QWidget>

#include "lang.h"

namespace {
const int kClientWidth = 720;
const int kClientHeight = 460;

QPushButton *makeButton(QWidget *parent, const QString &text)
{
    QPushButton *button = new QPushButton(text, parent);
    button->setFixedSize(85, 30);
    button->setCursor(Qt::PointingHandCursor);
    return button;
}

/// 使单元格只读且不可编辑
QTableWidgetItem *makeCell(const QString &text)
{
    QTableWidgetItem *item = new QTableWidgetItem(text);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    return item;
}
} // namespace

HistoryDialog::HistoryDialog(QWidget *parent)
    : QDialog(parent)
{
    m_records = ScanHistoryStore::load();

    setWindowTitle(Lang::get(QStringLiteral("HistoryTitle")));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(kClientWidth, kClientHeight);
    setStyleSheet(QStringLiteral("QDialog { background-color: white; }"));
    setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    setWindowIcon(QIcon(QStringLiteral(":/app.ico")));

    QVBoxLayout *rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    // ------------------------------------------------------------ 顶部提示
    m_lblHint = new QLabel(Lang::get(QStringLiteral("HistoryHint")), this);
    m_lblHint->setFixedHeight(34);
    m_lblHint->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_lblHint->setIndent(12);
    {
        QFont font(QStringLiteral("Microsoft YaHei"));
        font.setPointSizeF(8.5);
        m_lblHint->setFont(font);
        QPalette palette = m_lblHint->palette();
        palette.setColor(QPalette::WindowText, QColor(150, 150, 150));
        m_lblHint->setPalette(palette);
    }
    rootLayout->addWidget(m_lblHint, 0);

    // ------------------------------------------------------------ 历史列表
    QWidget *listContainer = new QWidget(this);
    QVBoxLayout *listLayout = new QVBoxLayout(listContainer);
    listLayout->setContentsMargins(12, 6, 12, 6);
    listLayout->setSpacing(0);

    m_table = new QTableWidget(listContainer);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels({Lang::get(QStringLiteral("ColHistoryTime")),
                                        Lang::get(QStringLiteral("ColHistoryRange")),
                                        Lang::get(QStringLiteral("ColHistoryTotal")),
                                        Lang::get(QStringLiteral("ColHistoryOnline")),
                                        Lang::get(QStringLiteral("ColHistoryDhcp"))});
    m_table->verticalHeader()->setVisible(false);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setShowGrid(true);
    m_table->setWordWrap(false);
    m_table->setFrameShape(QFrame::Box);
    {
        QFont font(QStringLiteral("Microsoft YaHei"));
        font.setPointSizeF(9.5);
        m_table->setFont(font);
    }
    QHeaderView *header = m_table->horizontalHeader();
    header->setSectionResizeMode(QHeaderView::Fixed);
    header->setStretchLastSection(false);
    m_table->setColumnWidth(0, 130);
    m_table->setColumnWidth(1, 260);
    m_table->setColumnWidth(2, 90);
    m_table->setColumnWidth(3, 90);
    m_table->setColumnWidth(4, 120);
    {
        // HideSelection = false —— 失去焦点时仍保持选中高亮
        QPalette palette = m_table->palette();
        palette.setColor(QPalette::Inactive, QPalette::Highlight,
                         palette.color(QPalette::Active, QPalette::Highlight));
        palette.setColor(QPalette::Inactive, QPalette::HighlightedText,
                         palette.color(QPalette::Active, QPalette::HighlightedText));
        m_table->setPalette(palette);
    }

    m_lblEmpty = new QLabel(Lang::get(QStringLiteral("HistoryEmpty")), listContainer);
    m_lblEmpty->setAlignment(Qt::AlignCenter);
    {
        QFont font(QStringLiteral("Microsoft YaHei"), 11);
        m_lblEmpty->setFont(font);
        QPalette palette = m_lblEmpty->palette();
        palette.setColor(QPalette::WindowText, QColor(170, 170, 170));
        m_lblEmpty->setPalette(palette);
    }

    listLayout->addWidget(m_table, 1);
    listLayout->addWidget(m_lblEmpty, 1);
    rootLayout->addWidget(listContainer, 1);

    // ------------------------------------------------------------ 底部按钮
    QWidget *buttonPanel = new QWidget(this);
    buttonPanel->setFixedHeight(52);
    QHBoxLayout *buttonLayout = new QHBoxLayout(buttonPanel);
    buttonLayout->setContentsMargins(12, 8, 12, 8);
    buttonLayout->setSpacing(8);

    m_btnView = makeButton(buttonPanel, Lang::get(QStringLiteral("HistoryView")));
    m_btnDelete = makeButton(buttonPanel, Lang::get(QStringLiteral("HistoryDelete")));
    m_btnClear = makeButton(buttonPanel, Lang::get(QStringLiteral("HistoryClear")));
    m_btnClose = makeButton(buttonPanel, Lang::get(QStringLiteral("Close")));

    buttonLayout->addStretch(1);
    buttonLayout->addWidget(m_btnView);
    buttonLayout->addWidget(m_btnDelete);
    buttonLayout->addWidget(m_btnClear);
    buttonLayout->addWidget(m_btnClose);

    rootLayout->addWidget(buttonPanel, 0);

    connect(m_btnView, &QPushButton::clicked, this, &HistoryDialog::viewSelected);
    connect(m_btnDelete, &QPushButton::clicked, this, &HistoryDialog::deleteSelected);
    connect(m_btnClear, &QPushButton::clicked, this, &HistoryDialog::clearAll);
    connect(m_btnClose, &QPushButton::clicked, this, &QDialog::reject);
    connect(m_table, &QTableWidget::cellDoubleClicked, this,
            [this](int, int) { viewSelected(); });

    reloadList();
}

void HistoryDialog::reloadList()
{
    m_table->clearContents();
    m_table->setRowCount(m_records.size());

    for (int row = 0; row < m_records.size(); ++row) {
        const ScanHistoryRecord &record = m_records.at(row);
        int total = 0;
        int online = 0;
        int dhcp = 0;
        for (const ScanHistoryDevice &device : record.devices) {
            ++total;
            if (device.isActive)
                ++online;
            if (device.isDhcpServer)
                ++dhcp;
        }

        m_table->setItem(row, 0,
                         makeCell(record.scanTime.toString(
                             Lang::get(QStringLiteral("HistoryDateFormat")))));
        m_table->setItem(row, 1,
                         makeCell(Lang::fmt(QStringLiteral("HistoryRangeFormat"),
                                            record.startIp, record.endIp)));
        m_table->setItem(row, 2, makeCell(QString::number(total)));
        m_table->setItem(row, 3, makeCell(QString::number(online)));
        m_table->setItem(row, 4, makeCell(QString::number(dhcp)));
    }

    const bool empty = m_records.isEmpty();
    m_lblEmpty->setVisible(empty);
    m_table->setVisible(!empty);
}

void HistoryDialog::viewSelected()
{
    const int row = m_table->currentRow();
    if (m_table->selectedItems().isEmpty() || row < 0 || row >= m_records.size()) {
        QMessageBox::information(this, Lang::get(QStringLiteral("Tip")),
                                 Lang::get(QStringLiteral("HistorySelectFirst")));
        return;
    }
    m_selectedRecord = m_records.at(row);
    accept();
}

void HistoryDialog::deleteSelected()
{
    const int row = m_table->currentRow();
    if (m_table->selectedItems().isEmpty() || row < 0 || row >= m_records.size()) {
        QMessageBox::information(this, Lang::get(QStringLiteral("Tip")),
                                 Lang::get(QStringLiteral("HistorySelectFirst")));
        return;
    }
    if (QMessageBox::question(this, Lang::get(QStringLiteral("Confirm")),
                              Lang::get(QStringLiteral("HistoryConfirmDelete")),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
        != QMessageBox::Yes) {
        return;
    }

    ScanHistoryStore::remove(m_records.at(row));
    m_records.removeAt(row);
    reloadList();
}

void HistoryDialog::clearAll()
{
    if (m_records.isEmpty())
        return;
    if (QMessageBox::question(this, Lang::get(QStringLiteral("Confirm")),
                              Lang::get(QStringLiteral("HistoryConfirmClear")),
                              QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes)
        != QMessageBox::Yes) {
        return;
    }

    ScanHistoryStore::clear();
    m_records.clear();
    reloadList();
}
