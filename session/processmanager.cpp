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

#include <KWindowSystem>

Q_LOGGING_CATEGORY(PM, "ProcessManager")

ProcessManager::ProcessManager(QObject *parent)
    : QObject(parent)
    , m_wmStarted(false)
    , m_waitLoop(nullptr)
{
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
    qCInfo(PM) << "Starting ProcessManager";
    startWindowManager();
    loadSystemProcess();

    QTimer::singleShot(100, this, &ProcessManager::loadAutoStartProcess);
}

void ProcessManager::logout()
{
    qCInfo(PM) << "Logging out";
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
    qCInfo(PM) << "Starting window manager";

    QProcess *wmProcess = new QProcess;
    // 启动Wayland窗口管理器
    wmProcess->start("kwin_wayland", QStringList());

    // 初始化等待循环
    m_waitLoop = new QEventLoop(this);

    // 添加超时以避免无限阻塞，如果WM无法执行
    QTimer::singleShot(30 * 1000, m_waitLoop, &QEventLoop::quit);
    m_waitLoop->exec();

    qCInfo(PM) << "Window manager started or timeout occurred";

    delete m_waitLoop;
    m_waitLoop = nullptr;
}

void ProcessManager::loadAutoStartProcess()
{
    qCInfo(PM) << "Loading auto start processes from configuration file";

    QString configFilePath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation) + "/autostart.conf";
    QFile configFile(configFilePath);

    if (!configFile.exists()) {
        qCWarning(PM) << "Configuration file not found:" << configFilePath;
        return;
    }

    if (!configFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qCWarning(PM) << "Unable to open configuration file:" << configFilePath;
        return;
    }

    QTextStream in(&configFile);
    QStringList execList;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (!line.isEmpty() && !line.startsWith("#")) {  // 忽略空行和注释
            execList << line;
        }
    }

    configFile.close();

    for (const QString &exec : execList) {
        QProcess *process = new QProcess;
        QStringList arguments = QProcess::splitCommand(exec);

        if (!arguments.isEmpty()) {
            process->setProgram(arguments.takeFirst());
            process->setArguments(arguments);
            process->start();
            process->waitForStarted();

            if (process->exitCode() == 0) {
                m_autoStartProcess.insert(exec, process);
            } else {
                process->deleteLater();
            }
        }
    }
}





void ProcessManager::loadAutoStartProcess()
{
    qCInfo(PM) << "Loading auto start processes";

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
