#include "process.h"

Process::Process(QObject *parent)
    : QProcess(parent)
{
    init();
}

Process::~Process() = default;

void Process::init()
{
    setProcessChannelMode(QProcess::ForwardedChannels);
}
