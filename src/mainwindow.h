#pragma once

#include "scanhistory.h"
#include "scanner.h"

#include <QVector>
#include <QWidget>

class QDialog;
class QLabel;
class QProgressBar;
class QPushButton;
class QTabWidget;
class QCloseEvent;
class QResizeEvent;
class QShowEvent;
class QWinTaskbarButton;
class QWinTaskbarProgress;

class IpAddressEdit;
class SubnetResultPanel;

/// 主窗口
class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void closeEvent(QCloseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void showEvent(QShowEvent *event) override;

private slots:
    void onScanClicked();
    void onStopClicked();
    void onClearClicked();
    void onExportClicked();
    void onSettingsClicked();
    void onHistoryClicked();

    void onScanProgress(int progress);
    void onScanCompleted(const QVector<DhcpServerInfo> &results);
    void onScanError(const QString &message);
    void onScanFinished();

private:
    void buildUi();
    void applyLanguage();
    void layoutSearchPanel();
    void setScanningUiEnabled(bool scanning);
    void populateResultTabs(const QVector<DhcpServerInfo> &results);
    void saveScanHistory(const QVector<DhcpServerInfo> &results);
    void displayHistoryRecord(const ScanHistoryRecord &record);
    bool exportToCsv(const QString &filePath, QString *errorMessage);
    void showTooManySubnetsMessage(int count);
    void showProgressDialog();
    void closeProgressDialog();

    // 搜索面板
    QWidget *m_panelSearch = nullptr;
    QLabel *m_labelTitle = nullptr;
    QLabel *m_labelStartIp = nullptr;
    IpAddressEdit *m_ipStart = nullptr;
    QLabel *m_labelTo = nullptr;
    QLabel *m_labelEndIp = nullptr;
    IpAddressEdit *m_ipEnd = nullptr;
    QWidget *m_flowButtons = nullptr;
    QPushButton *m_buttonScan = nullptr;
    QPushButton *m_buttonStop = nullptr;
    QPushButton *m_buttonClear = nullptr;
    QPushButton *m_buttonExport = nullptr;
    QPushButton *m_buttonSettings = nullptr;
    QPushButton *m_buttonHistory = nullptr;

    // 进度（显示在 Windows 任务栏图标上）与结果
    QWinTaskbarButton *m_taskbarButton = nullptr;
    QWinTaskbarProgress *m_taskbarProgress = nullptr;
    QTabWidget *m_tabControlResults = nullptr;

    // 扫描进度弹窗（带百分比）
    QDialog *m_progressDialog = nullptr;
    QLabel *m_progressRangeLabel = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QLabel *m_progressHintLabel = nullptr;
    QPushButton *m_buttonStopInDialog = nullptr;

    // 状态栏
    QWidget *m_panelBottom = nullptr;
    QLabel *m_statusLabel = nullptr;
    QLabel *m_statusCount = nullptr;

    Scanner *m_scanner = nullptr;
    bool m_isScanning = false;
    QString m_currentStartIp;
    QString m_currentEndIp;
};
