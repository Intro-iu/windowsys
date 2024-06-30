#include "application.h"
#include <QQuickWindow>

int main(int argc, char *argv[])
{
    qputenv("QT_QPA_PLATFORM", QByteArrayLiteral("wayland"));

    QQuickWindow::setDefaultAlphaBuffer(true);
    // 如果应用程序在Wayland下高DPI表现正常，可以移除禁用高DPI缩放的属性设置。
    // QCoreApplication::setAttribute(Qt::AA_DisableHighDpiScaling);

    Application a(argc, argv);
    a.setQuitOnLastWindowClosed(false);

    return a.exec();
}
