#include "application.h"
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    // 清除SESSION_MANAGER环境变量
    putenv((char *)"SESSION_MANAGER=");

    // 设置QT_QPA_PLATFORM为wayland
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));

    QQuickWindow::setDefaultAlphaBuffer(true);
    QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    Application a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    qDebug() << "Starting session application";

    return a.exec();
}
