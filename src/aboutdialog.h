#pragma once

#include <QDialog>

/// 关于窗口（白色背景 + 左侧 Logo + 右侧文字信息，整体自绘）。
class AboutDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AboutDialog(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
};
