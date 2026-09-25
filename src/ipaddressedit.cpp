#include "ipaddressedit.h"

#include <QAbstractSocket>
#include <QHBoxLayout>
#include <QHostAddress>
#include <QIntValidator>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QTimer>

namespace
{
const int kSegmentCount = 4;
const int kMaxSegmentLength = 3;
} // namespace

IpAddressEdit::IpAddressEdit(QWidget *parent)
    : QWidget(parent)
{
    setFocusPolicy(Qt::NoFocus);
    setAutoFillBackground(false);

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 1, 4, 1);
    layout->setSpacing(0);

    for (int i = 0; i < kSegmentCount; ++i)
    {
        QLineEdit *segment = new QLineEdit(this);
        segment->setValidator(new QIntValidator(0, 255, segment));
        segment->setMaxLength(kMaxSegmentLength);
        segment->setAlignment(Qt::AlignCenter);
        segment->setFrame(false);
        segment->setMinimumWidth(16);
        segment->setStyleSheet(
            QStringLiteral("QLineEdit { border: none; background: transparent; }"));
        segment->installEventFilter(this);
        connect(segment, &QLineEdit::textEdited, this,
                [this, i](const QString &text) {
                    // 输满 3 位自动跳到下一段
                    if (text.size() >= kMaxSegmentLength && i != kSegmentCount - 1)
                        focusSegment(i + 1);
                    emit addressChanged();
                });
        m_segments[i] = segment;
        layout->addWidget(segment, 1);

        if (i != kSegmentCount - 1)
        {
            QLabel *dot = new QLabel(QStringLiteral("."), this);
            dot->setAlignment(Qt::AlignCenter);
            dot->setFixedWidth(7);
            dot->setStyleSheet(QStringLiteral("color: rgb(60,60,60);"));
            layout->addWidget(dot, 0);
        }
    }
}

QString IpAddressEdit::address() const
{
    QStringList parts;
    parts.reserve(kSegmentCount);
    for (int i = 0; i < kSegmentCount; ++i)
    {
        const QString text = m_segments[i]->text();
        if (text.isEmpty())
            return QString();
        parts.append(text);
    }
    return parts.join(QLatin1Char('.'));
}

void IpAddressEdit::setAddress(const QString &ip)
{
    QHostAddress address;
    if (!address.setAddress(ip.trimmed())
        || address.protocol() != QAbstractSocket::IPv4Protocol)
    {
        return;
    }

    const QStringList parts = address.toString().split(QLatin1Char('.'));
    if (parts.size() != kSegmentCount)
        return;

    for (int i = 0; i < kSegmentCount; ++i)
        m_segments[i]->setText(parts.at(i));
}

QSize IpAddressEdit::sizeHint() const
{
    return QSize(248, 28);
}

int IpAddressEdit::indexOfSegment(const QObject *object) const
{
    for (int i = 0; i < kSegmentCount; ++i)
    {
        if (m_segments[i] == object)
            return i;
    }
    return -1;
}

void IpAddressEdit::focusSegment(int index)
{
    if (index < 0 || index >= kSegmentCount)
        return;

    QLineEdit *segment = m_segments[index];
    segment->setFocus();
    segment->selectAll();
}

bool IpAddressEdit::eventFilter(QObject *watched, QEvent *event)
{
    QLineEdit *segment = qobject_cast<QLineEdit *>(watched);
    const int index = indexOfSegment(watched);
    if (segment == nullptr || index < 0)
        return QWidget::eventFilter(watched, event);

    switch (event->type())
    {
    case QEvent::KeyPress:
    {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        // 输入点号（或逗号）自动跳到下一段
        if (keyEvent->key() == Qt::Key_Period || keyEvent->key() == Qt::Key_Comma)
        {
            focusSegment(index + 1);
            return true;
        }
        // 在当前段起始位置按退格，跳回上一段
        if (keyEvent->key() == Qt::Key_Backspace && segment->text().isEmpty())
        {
            focusSegment(index - 1);
            return true;
        }
        break;
    }
    case QEvent::FocusIn:
        // 获得焦点时全选（对应原生控件的 SelectAll 行为）
        QTimer::singleShot(0, segment, &QLineEdit::selectAll);
        break;
    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

void IpAddressEdit::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::white);
    painter.setPen(QColor(120, 120, 120));
    painter.drawRect(rect().adjusted(0, 0, -1, -1));
}
