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
#include <syslog.h>

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
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
        syslog(LOG_CRIT, "Fatal: %s (%s:%u, %s)", localMsg.constData(), file, context.line, function);
        abort();
    }
}

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
    , m_wmStarted(false)
    , m_waitLoop(nullptr)
{
    // Wayland doesn't require a native event filter
    qInstallMessageHandler(customMessageHandler);
    qDebug() << "ProcessManager created";
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

    QEventLoop waitLoop;
    m_waitLoop = &waitLoop;
    // add a timeout to avoid infinite blocking if a WM fail to execute.
    QTimer::singleShot(30 * 1000, &waitLoop, SLOT(quit()));
    waitLoop.exec();
    m_waitLoop = nullptr;

    if (wmProcess->state() == QProcess::Running) {
        qDebug() << "Window manager started successfully";
        m_wmStarted = true;
    } else {
        qDebug() << "Failed to start window manager";
    }
}

void ProcessManager::loadSystemProcess()
{
    QList<QPair<QString, QStringList>> list;
    list << qMakePair(QString("firefox"), QStringList());

    for (QPair<QString, QStringList> pair : list) {
    QProcess *process = new QProcess;
    process->setProcessChannelMode(QProcess::ForwardedChannels);
    process->setProgram(pair.first);
    process->setArguments(pair.second);
    process->start();
    if (!process->waitForStarted()) {
        qDebug() << "Failed to start process:" << pair.first << process->errorString();
        delete process;
        continue;
    } else qDebug() << "Load DE components: " << pair.first << pair.second;

    connect(process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), 
                [pair](int exitCode, QProcess::ExitStatus exitStatus) {
            qDebug() << "Process finished:" << pair.first << "Exit code:" << exitCode << "Exit status:" << exitStatus;
        });

    // if (pair.first == "cutefish-settings-daemon") {
    //     QThread::msleep(800);
    // }

    // Add to map
    if (process->exitCode() == 0) {
        m_autoStartProcess.insert(pair.first, process);
    } else {
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
        process->waitForStarted();

        if (process->exitCode() == 0) {
            m_autoStartProcess.insert(exec, process);
        } else {
            process->deleteLater();
        }
    }
}
