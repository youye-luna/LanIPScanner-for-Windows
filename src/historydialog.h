#pragma once

#include <QDialog>
#include <QVector>

#include "scanhistory.h"

class QLabel;
class QPushButton;
class QTableWidget;

/// 扫描历史窗口（查看/删除/清空历史扫描记录）。
class HistoryDialog : public QDialog
{
    Q_OBJECT
public:
    explicit HistoryDialog(QWidget *parent = nullptr);

    /// 用户选择要查看的历史记录（点击“查看”或双击时设置）
    ScanHistoryRecord selectedRecord() const { return m_selectedRecord; }

private:
    void reloadList();
    void viewSelected();
    void deleteSelected();
    void clearAll();

    QVector<ScanHistoryRecord> m_records;
    ScanHistoryRecord m_selectedRecord;

    QLabel *m_lblHint = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_lblEmpty = nullptr;
    QPushButton *m_btnView = nullptr;
    QPushButton *m_btnDelete = nullptr;
    QPushButton *m_btnClear = nullptr;
    QPushButton *m_btnClose = nullptr;
};
