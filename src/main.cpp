#include "mainwindow.h"
#include "scanner.h"

#include <QApplication>
#include <QFont>
#include <QIcon>
#include <QMetaType>

int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("LanIPScanner"));
    QApplication::setApplicationVersion(QStringLiteral("1.5-beta2"));
    QApplication::setWindowIcon(QIcon(QStringLiteral(":/app.ico")));
    app.setFont(QFont(QStringLiteral("Microsoft YaHei"), 9));

    // 跨线程信号需要注册自定义类型
    qRegisterMetaType<DhcpServerInfo>("DhcpServerInfo");
    qRegisterMetaType<QVector<DhcpServerInfo>>("QVector<DhcpServerInfo>");

    MainWindow window;
    window.show();

    return app.exec();
}
