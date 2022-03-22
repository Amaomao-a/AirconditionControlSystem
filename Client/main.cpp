#include "client.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Client w;
    w.show();

    QThread *backThread = new QThread;   // 后台线程
    workThread *work = new workThread();
    work->moveToThread(backThread);       // 将工作类移入后台线程
    backThread->start();

    QObject::connect(&w, &Client::systemStart, work, &workThread::workStart);     // 发出信号时才开始工作
    QObject::connect(&w, &Client::systemStop, work, &workThread::workStop);
    QObject::connect(work, &workThread::tempChange, &w, &Client::tempUpdate); // 温度实时更新
    QObject::connect(&w, &Client::windChange, work, &workThread::workStart);     // 风速改变时需重启定时器
    QObject::connect(work, &workThread::tempUp, &w, &Client::tempRebound);
    QObject::connect(work, &workThread::tempRelease, &w, &Client::tempRelease);

    return a.exec();
}
