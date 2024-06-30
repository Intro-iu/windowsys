#ifndef PROCESS_H
#define PROCESS_H

#include <QProcess>

class Process : public QProcess
{
    Q_OBJECT

public:
    explicit Process(QObject *parent = nullptr);
    ~Process() override;

private:
    void init();
};

#endif // PROCESS_H
