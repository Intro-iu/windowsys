#include "processmanager.h"

#include <QCoreApplication>
#include <QStandardPaths>
#include <QFileInfoList>
#include <QFileInfo>
#include <QSettings>
#include <QDebug>
#include <QTimer>
#include <QThread>
#include <QDir>

#include <KWindowSystem>

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

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
    , m_wmStarted(false)
    , m_waitLoop(nullptr)
{
    openlog("prts-session", LOG_PID | LOG_CONS, LOG_USER);
    qInstallMessageHandler(messageHandler);
}

ProcessManager::~ProcessManager()
{
    QMapIterator<QString, QProcess *> i(m_systemProcess);
    while (i.hasNext()) {
        i.next();
        QProcess *p = i.value();
        delete p;
        m_systemProcess[i.key()] = nullptr;
    }
}

void ProcessManager::start()
{
    startWindowManager();
    loadSystemProcess();

    QTimer::singleShot(100, this, &ProcessManager::loadAutoStartProcess);
}

void ProcessManager::logout()
{
    QMapIterator<QString, QProcess *> i(m_systemProcess);

    while (i.hasNext()) {
        i.next();
        QProcess *p = i.value();
        p->terminate();
    }
    i.toFront();

    while (i.hasNext()) {
        i.next();
        QProcess *p = i.value();
        if (p->state() != QProcess::NotRunning && !p->waitForFinished(2000)) {
            p->kill();
        }
    }

    QCoreApplication::exit(0);
}

void ProcessManager::startWindowManager()
{
    qDebug() << "Starting window manager";

    QProcess *wmProcess = new QProcess;
    wmProcess->start("kwin_wayland", QStringList());

    if (!wmProcess->waitForStarted()) {
        qCritical() << "Failed to start window manager process";
        return;
    }

    qDebug() << "Window manager process started, waiting for it to initialize";

    m_waitLoop = new QEventLoop(this);
    QTimer::singleShot(30 * 1000, m_waitLoop, &QEventLoop::quit);
    m_waitLoop->exec();

    if (wmProcess->state() == QProcess::Running) {
        qDebug() << "Window manager started successfully";
    } else {
        qCritical() << "Window manager did not start within the timeout period";
    }

    delete m_waitLoop;
    m_waitLoop = nullptr;
}


void ProcessManager::loadSystemProcess()
{
    QList<QPair<QString, QStringList>> list;
    // 仅启动 Firefox
    list << qMakePair(QString("firefox"), QStringList());

    for (QPair<QString, QStringList> pair : list) {
        QProcess *process = new QProcess;
        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->setProgram(pair.first);
        process->setArguments(pair.second);
        process->start();

        if (!process->waitForStarted()) {
            qCritical() << "Failed to start process:" << pair.first;
            process->deleteLater();
            continue;
        }

        qDebug() << "Load DE components: " << pair.first << pair.second;

        if (process->state() == QProcess::Running) {
            qDebug() << pair.first << " is running.";
            m_autoStartProcess.insert(pair.first, process);
        } else {
            qCritical() << pair.first << " failed to start.";
            process->deleteLater();
        }
    }
}


void ProcessManager::loadAutoStartProcess()
{
    QStringList execList;
    const QStringList dirs = QStandardPaths::locateAll(QStandardPaths::GenericConfigLocation,
                                                       QStringLiteral("autostart"),
                                                       QStandardPaths::LocateDirectory);
    for (const QString &dir : dirs) {
        const QDir d(dir);
        const QStringList fileNames = d.entryList(QStringList() << QStringLiteral("*.desktop"));
        for (const QString &file : fileNames) {
            QSettings desktop(d.absoluteFilePath(file), QSettings::IniFormat);
            desktop.beginGroup("Desktop Entry");

            if (desktop.contains("OnlyShowIn"))
                continue;

            const QString execValue = desktop.value("Exec").toString();

            if (!execValue.isEmpty()) {
                execList << execValue;
            }
        }
    }

    for (const QString &exec : execList) {
        QProcess *process = new QProcess;
        process->setProgram(exec);
        process->start();

        if (!process->waitForStarted()) {
            qCritical() << "Failed to start autostart process:" << exec;
            process->deleteLater();
            continue;
        }

        if (process->state() == QProcess::Running) {
            m_autoStartProcess.insert(exec, process);
        } else {
            process->deleteLater();
        }
    }
}
