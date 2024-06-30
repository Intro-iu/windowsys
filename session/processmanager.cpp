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
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(LOG_PROCESS_MANAGER, "processmanager")

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
    , m_wmStarted(false)
    , m_waitLoop(nullptr)
{
    qCInfo(LOG_PROCESS_MANAGER) << "ProcessManager initialized";
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
    qCInfo(LOG_PROCESS_MANAGER) << "Starting ProcessManager";
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
    qCInfo(LOG_PROCESS_MANAGER) << "Starting window manager";
    QProcess *wmProcess = new QProcess;
    wmProcess->setProcessChannelMode(QProcess::ForwardedChannels);
    wmProcess->start("kwin_wayland", QStringList());

    QEventLoop waitLoop;
    m_waitLoop = &waitLoop;
    // add a timeout to avoid infinite blocking if a WM fail to execute.
    QTimer::singleShot(30 * 1000, &waitLoop, SLOT(quit()));
    waitLoop.exec();
    m_waitLoop = nullptr;

    if (wmProcess->state() == QProcess::Running) {
        qCInfo(LOG_PROCESS_MANAGER) << "Window manager started successfully";
        m_wmStarted = true;
    } else {
        qCWarning(LOG_PROCESS_MANAGER) << "Failed to start window manager";
    }
}

void ProcessManager::loadSystemProcess()
{
    qCInfo(LOG_PROCESS_MANAGER) << "Loading system processes";
    QList<QPair<QString, QStringList>> list;
    list << qMakePair(QString("cutefish-settings-daemon"), QStringList());
    list << qMakePair(QString("cutefish-xembedsniproxy"), QStringList());

    // Desktop components
    list << qMakePair(QString("cutefish-filemanager"), QStringList("--desktop"));
    list << qMakePair(QString("cutefish-statusbar"), QStringList());
    list << qMakePair(QString("cutefish-dock"), QStringList());
    list << qMakePair(QString("cutefish-launcher"), QStringList());

    list << qMakePair(QString("firefox"), QStringList());

    for (QPair<QString, QStringList> pair : list) {
        QProcess *process = new QProcess;
        process->setProcessChannelMode(QProcess::ForwardedChannels);
        process->setProgram(pair.first);
        process->setArguments(pair.second);
        process->start();
        process->waitForStarted();

        if (pair.first == "cutefish-settings-daemon") {
            QThread::msleep(800);
        }

        qCInfo(LOG_PROCESS_MANAGER) << "Load DE components: " << pair.first << pair.second;

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
    qCInfo(LOG_PROCESS_MANAGER) << "Loading auto start processes";
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
        process->setProcessChannelMode(QProcess::ForwardedChannels);
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
