#include <QGuiApplication>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QtWaylandCompositor/QWaylandCompositor>
#include <QtWaylandCompositor/QWaylandQuickCompositor>
#include <QtQuick/QQuickView>

class Compositor : public QWaylandQuickCompositor
{
    Q_OBJECT
public:
    Compositor() {
        setBufferSwapBehavior(QWaylandQuickCompositor::DoubleBuffered);
        create();
    }

protected:
    void create() {
        QWaylandCompositor::create();
    }
};

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.addHelpOption();
    parser.process(app);

    Compositor compositor;

    QQuickView view;
    view.setResizeMode(QQuickView::SizeRootObjectToView);
    view.setSource(QUrl(QStringLiteral("qrc:/main.qml")));
    view.show();

    return app.exec();
}

#include "main.moc"
