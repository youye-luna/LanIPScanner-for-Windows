#include "aboutdialog.h"

#include <QDate>
#include <QApplication>
#include <QFont>
#include <QFontMetricsF>
#include <QIcon>
#include <QPainter>

#include "lang.h"

namespace {
const int kClientWidth = 500;
const int kClientHeight = 384;

// 内容区左边距与顶部基线
const int kContentX = 38;
const int kContentY = 30;

const QColor kNameColor(50, 50, 50);
const QColor kTitleColor(130, 130, 130);
const QColor kLineColor(230, 232, 235);
const QColor kVersionColor(150, 150, 150);
const QColor kFeatureColor(100, 100, 100);
const QColor kCopyrightColor(190, 190, 190);
} // namespace

AboutDialog::AboutDialog(QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(Lang::get(QStringLiteral("AboutTitle")));
    setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    setFixedSize(kClientWidth, kClientHeight);
    setStyleSheet(QStringLiteral("QDialog { background-color: white; }"));
    setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));
    setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
}

void AboutDialog::paintEvent(QPaintEvent *event)
{
    QDialog::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::TextAntialiasing, true);

    const int cx = width() / 2;

    // === 文字信息 ===
    const int textX = kContentX;
    const int textWidth = kClientWidth - textX - 40;

    // LanIPScanner（英文名）
    QFont nameFont(QStringLiteral("Segoe UI Semibold"), 17);
    nameFont.setBold(true);
    painter.setFont(nameFont);
    painter.setPen(kNameColor);
    {
        const QFontMetricsF metrics(nameFont);
        painter.drawText(QPointF(textX, kContentY + 6 + metrics.ascent()),
                         QStringLiteral("LanIPScanner"));
    }

    // 局域网设备扫描工具（中文名）
    QFont cnFont(QStringLiteral("Microsoft YaHei"), 11);
    painter.setFont(cnFont);
    painter.setPen(kTitleColor);
    {
        const QFontMetricsF metrics(cnFont);
        painter.drawText(QPointF(textX, kContentY + 42 + metrics.ascent()),
                         Lang::get(QStringLiteral("FormTitle")));
    }

    // 分隔线
    painter.setPen(QPen(kLineColor, 1));
    painter.drawLine(textX, kContentY + 76, textX + textWidth, kContentY + 76);

    // 版本号
    QFont verFont(QStringLiteral("Microsoft YaHei"), 9);
    verFont.setPointSizeF(9.5);
    painter.setFont(verFont);
    painter.setPen(kVersionColor);
    {
        const QFontMetricsF metrics(verFont);
        painter.drawText(QPointF(textX, kContentY + 90 + metrics.ascent()),
                         Lang::fmt(QStringLiteral("AboutVersion"),
                                   QApplication::applicationVersion()));
    }

    // 功能简介
    QFont descFont(QStringLiteral("Microsoft YaHei"), 8);
    descFont.setPointSizeF(8.5);
    painter.setFont(descFont);
    painter.setPen(kFeatureColor);
    {
        const QFontMetricsF metrics(descFont);
        int descY = kContentY + 120;
        const QStringList features =
            Lang::get(QStringLiteral("AboutFeatures")).split(QLatin1Char('|'));
        for (const QString &feat : features) {
            painter.drawText(QPointF(textX, descY + metrics.ascent()),
                             QStringLiteral("  ") + feat);
            descY += 18;
        }
    }

    // 开源组件声明（Qt 社区版 / nmap）
    QFont srcFont(QStringLiteral("Microsoft YaHei"), 8);
    srcFont.setPointSizeF(8.5);
    {
        const int srcTop = kContentY + 220;

        painter.setPen(QPen(kLineColor, 1));
        painter.drawLine(textX, srcTop, textX + textWidth, srcTop);

        const QFontMetricsF metrics(srcFont);
        painter.setFont(srcFont);
        painter.setPen(kTitleColor);
        painter.drawText(QPointF(textX, srcTop + 16 + metrics.ascent()),
                         Lang::get(QStringLiteral("AboutOpenSource")));

        painter.setPen(kFeatureColor);
        painter.drawText(QPointF(textX, srcTop + 34 + metrics.ascent()),
                         QStringLiteral("  ") + Lang::get(QStringLiteral("AboutQtLicense")));
        painter.drawText(QPointF(textX, srcTop + 52 + metrics.ascent()),
                         QStringLiteral("  ") + Lang::get(QStringLiteral("AboutNmapLicense")));
    }

    // 版权（底部居中）
    QFont copyFont(QStringLiteral("Microsoft YaHei"), 8);
    copyFont.setPointSizeF(8.5);
    painter.setFont(copyFont);
    painter.setPen(kCopyrightColor);
    {
        const QString copyText = QStringLiteral("Copyright \u00A9 %1 %2")
                                     .arg(QDate::currentDate().year())
                                     .arg(QStringLiteral("youye-luna"));
        const QFontMetricsF metrics(copyFont);
        painter.drawText(QPointF(cx - metrics.horizontalAdvance(copyText) / 2.0,
                                 348 + metrics.ascent()),
                         copyText);
    }
}
