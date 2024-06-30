#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QWaylandCompositor>
#include <QWaylandQuickCompositor>

class SimpleCompositor : public QWaylandQuickCompositor {
    Q_OBJECT
public:
    SimpleCompositor() : QWaylandQuickCompositor() {
        setOutputGeometry(QRect(0, 0, 800, 600));
    }
};

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    SimpleCompositor compositor;
    compositor.create();

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("compositor", &compositor);
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
