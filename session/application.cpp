#include "application.h"
#include "sessionadaptor.h"

#include <QDBusConnection>
#include <QStandardPaths>
#include <QSettings>
#include <QProcess>
#include <QDebug>
#include <QDir>
#include <QTimer>
#include <QLoggingCategory>
#include <QtMessageHandler>
#include <syslog.h>
#include <exception>

// Function to handle logging to syslog
void messageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    const char *file = context.file ? context.file : "";
    const char *function = context.function ? context.function : "";
    switch (type) {
        case QtDebugMsg:
            syslog(LOG_DEBUG, "Debug: %s (%s:%u, %s)", localMsg.constData(), file, context.line, function);
            break;
        case QtInfoMsg:
            syslog(LOG_INFO, "Info: %s (%s:%u, %s)", localMsg.constData(), file, context.line, function);
            break;
        case QtWarningMsg:
            syslog(LOG_WARNING, "Warning: %s (%s:%u, %s)", localMsg.constData(), file, context.line, function);
            break;
        case QtCriticalMsg:
            syslog(LOG_CRIT, "Critical: %s (%s:%u, %s)", localMsg.constData(), file, context.line, function);
            break;
        case QtFatalMsg:
            syslog(LOG_ALERT, "Fatal: %s (%s:%u, %s)", localMsg.constData(), file, context.line, function);
            abort();
    }
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_processManager(new ProcessManager)
{
    // Set up logging to syslog
    openlog("prts-session", LOG_PID | LOG_CONS, LOG_USER);
    qInstallMessageHandler(messageHandler);

    try {
        qDebug() << "Initializing session adaptor";
        new SessionAdaptor(this);

        qDebug() << "Registering DBus service";
        if (!QDBusConnection::sessionBus().registerService(QStringLiteral("org.cutefish.Session"))) {
            qWarning() << "Failed to register DBus service";
            return;
        }

        qDebug() << "Registering DBus object";
        if (!QDBusConnection::sessionBus().registerObject(QStringLiteral("/Session"), this)) {
            qWarning() << "Failed to register DBus object";
            return;
        }

        createConfigDirectory();
        initEnvironments();
        initLanguage();

        if (!syncDBusEnvironment()) {
            // Startup error
            qWarning() << "Could not sync environment to DBus.";
        } else {
            qDebug() << "Environment synced to DBus.";
        }

        QTimer::singleShot(100, m_processManager, &ProcessManager::start);
    } catch (const std::exception &e) {
        qWarning() << "Failed to initialize session: " << e.what();
    } catch (...) {
        qWarning() << "Failed to initialize session: Unknown error";
    }
}

void Application::initEnvironments()
{
    qDebug() << "Initializing environments";

    // Set defaults
    if (qEnvironmentVariableIsEmpty("XDG_DATA_HOME"))
        qputenv("XDG_DATA_HOME", QDir::home().absoluteFilePath(QStringLiteral(".local/share")).toLocal8Bit());
    if (qEnvironmentVariableIsEmpty("XDG_DESKTOP_DIR"))
        qputenv("XDG_DESKTOP_DIR", QDir::home().absoluteFilePath(QStringLiteral("/Desktop")).toLocal8Bit());
    if (qEnvironmentVariableIsEmpty("XDG_CONFIG_HOME"))
        qputenv("XDG_CONFIG_HOME", QDir::home().absoluteFilePath(QStringLiteral(".config")).toLocal8Bit());
    if (qEnvironmentVariableIsEmpty("XDG_CACHE_HOME"))
        qputenv("XDG_CACHE_HOME", QDir::home().absoluteFilePath(QStringLiteral(".cache")).toLocal8Bit());
    if (qEnvironmentVariableIsEmpty("XDG_DATA_DIRS"))
        qputenv("XDG_DATA_DIRS", "/usr/local/share/:/usr/share/");
    if (qEnvironmentVariableIsEmpty("XDG_CONFIG_DIRS"))
        qputenv("XDG_CONFIG_DIRS", "/etc/xdg");

    // Environment
    qputenv("DESKTOP_SESSION", "Cutefish");
    qputenv("XDG_CURRENT_DESKTOP", "Cutefish");
    qputenv("XDG_SESSION_DESKTOP", "Cutefish");

    // Qt
    qputenv("QT_QPA_PLATFORMTHEME", "cutefish");
    qputenv("QT_PLATFORM_PLUGIN", "cutefish");

    qunsetenv("QT_AUTO_SCREEN_SCALE_FACTOR");
    qunsetenv("QT_SCALE_FACTOR");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");
    qunsetenv("QT_ENABLE_HIGHDPI_SCALING");
    qunsetenv("QT_USE_PHYSICAL_DPI");
    qunsetenv("QT_FONT_DPI");
    qputenv("QT_SCALE_FACTOR_ROUNDING_POLICY", "PassThrough");

    // IM Config
    qputenv("GTK_IM_MODULE", "fcitx5");
    qputenv("QT4_IM_MODULE", "fcitx5");
    qputenv("QT_IM_MODULE", "fcitx5");
    qputenv("CLUTTER_IM_MODULE", "fcitx5");
    qputenv("XMODIFIERS", "@im=fcitx");
}

void Application::initLanguage()
{
    qDebug() << "Initializing language settings";

    QSettings settings(QSettings::UserScope, "cutefishos", "language");
    QString value = settings.value("language", "en_US").toString();
    QString str = QString("%1.UTF-8").arg(value);

    const auto lcValues = {
        "LANG", "LC_NUMERIC", "LC_TIME", "LC_MONETARY", "LC_MEASUREMENT", "LC_COLLATE", "LC_CTYPE"
    };

    for (auto lc : lcValues) {
        const QString value = str;
        if (!value.isEmpty()) {
            qputenv(lc, value.toUtf8());
        }
    }

    if (!value.isEmpty()) {
        qputenv("LANGUAGE", value.toUtf8());
    }
}

static bool isInteger(double x)
{
    int truncated = (int)x;
    return (x == truncated);
}

bool Application::syncDBusEnvironment()
{
    qDebug() << "Syncing environment to DBus";

    int exitCode = 0;

    // At this point all environment variables are set, let's send it to the DBus session server to update the activation environment
    if (!QStandardPaths::findExecutable(QStringLiteral("dbus-update-activation-environment")).isEmpty()) {
        exitCode = runSync(QStringLiteral("dbus-update-activation-environment"), { QStringLiteral("--systemd"), QStringLiteral("--all") });
    }

    return exitCode == 0;
}

void Application::createConfigDirectory()
{
    qDebug() << "Creating config directory";

    const QString configDir = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation);

    if (!QDir().mkpath(configDir)) {
        qWarning() << "Could not create config directory XDG_CONFIG_HOME: " << configDir;
    } else {
        qDebug() << "Config directory created: " << configDir;
    }
}

int Application::runSync(const QString &program, const QStringList &args, const QStringList &env)
{
    qDebug() << "Running program:" << program << "with arguments:" << args << "and environment:" << env;

    QProcess p;

    if (!env.isEmpty())
        p.setEnvironment(QProcess::systemEnvironment() << env);

    p.setProcessChannelMode(QProcess::ForwardedChannels);
    p.start(program, args);
    p.waitForFinished(-1);

    if (p.exitCode()) {
        qWarning() << program << args << "exited with code" << p.exitCode();
    } else {
        qDebug() << program << args << "finished successfully";
    }

    return p.exitCode();
}
