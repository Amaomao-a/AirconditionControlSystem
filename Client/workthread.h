#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include <QTimer>
#include <QObject>
#include <QThread>
#include <math.h>
#include <QDebug>

typedef struct room{
public:
    QString refId, token;
    QString roomID;
    QString ipAddress, port; // IP地址与端口

    int power;   // 1--ON, 0--OFF
    int mode;     // 工作模式(0--制冷，1--制热)，默认制冷
    int workWind, applyWind;     // 风速等级1~3 → LOW->MID->HIGH

    QPair<double, double> tempRange;  // 控温范围
    double indoorTmp; // 室温，也是空调工作的初始温度
    double initTmp;     // 室外温度
    double setTmp;     // 用户设定的目标温度
    double currentFee, totalFee;// 最近一次开机到现在的费用、当前累计费用
    double rateFee;  // 费率
}ROOM;
extern ROOM* myRoom;

class workThread : public QObject
{
    Q_OBJECT
private:
    QTimer *workTimer, *sleepTimer;
public:
    explicit workThread(QObject *parent=nullptr)
    {
        workTimer = new QTimer(this);
        sleepTimer = new QTimer(this);
        connect(workTimer, SIGNAL(timeout()), this, SLOT(tempControl()));
        connect(sleepTimer, SIGNAL(timeout()), this, SLOT(tempRebound()));

    }

public slots:
    void tempControl()  // 空调工作状态--控温
    {
        if(myRoom->power && myRoom->workWind
                && abs(myRoom->indoorTmp-myRoom->setTmp)>pow(10,-6))
        {
            if(myRoom->mode)    myRoom->indoorTmp += 0.1;
            else  myRoom->indoorTmp -= 0.1;
            qDebug() <<QString("完成一次降温! [indoorTemp: %1 °C, setTemp: %2 °C]").arg(myRoom->indoorTmp).arg(myRoom->setTmp);
            emit tempChange();
        }
        else
        {
            if(myRoom->power)   emit tempUp();     // 已经达到设定温度，空调休眠
            qDebug() << "空调休眠中..." << myRoom->power << myRoom->workWind;
            workTimer->stop();
        }
    }// tempControl

    void tempRebound() // 空调休眠状态--温度回升
    {
        if(myRoom->mode && myRoom->indoorTmp<=myRoom->initTmp)  return; // 制热模式下，自动回调的室温不能低于室外温度
        if(!myRoom->mode && myRoom->indoorTmp>=myRoom->initTmp) return; // 制冷模式下，自动回调的室温不能高于室外温度
        if(myRoom->power && myRoom->workWind)   return; // 如果空调正在工作，则不考虑环境温度改变。

        if(myRoom->mode)    myRoom->indoorTmp -= 0.1;
        else  myRoom->indoorTmp += 0.1;

        qDebug() <<QString("室温正在回升! [indoorTemp: %1 °C, setTemp: %2 °C]").arg(myRoom->indoorTmp).arg(myRoom->setTmp);
        if(abs(myRoom->indoorTmp-myRoom->setTmp)>=1)  // 超过1°C的温度波动范围，空调重启
        {
            qDebug() << "室温波动过大 空调已重启!";
            emit tempRelease();
            sleepTimer->stop();
        }
        emit tempChange();
    }// tempRebound

    void workStart()
    {
        if(myRoom->workWind==1)        // 工作状态：低风速，每分钟改变0.4°C，每0.1°C需时15s
            workTimer->start(1000 * 15);
        else if(myRoom->workWind==2) // 工作状态：中风速，每分钟改变0.5°C，每0.1°C需时12s
            workTimer->start(1000 * 12);
        else if(myRoom->workWind==3) // 工作状态：高风速，每分钟改变0.6°C，每0.1°C需时10s
            workTimer->start(1000 * 10);

        sleepTimer->start(1000 * 12);   // 休眠状态：室温回升，每分钟回弹0.5°C
    }
    void workStop()
    {
        if(workTimer->isActive())   workTimer->stop();
        if(sleepTimer->isActive())  sleepTimer->stop();
    }

signals:
    void tempChange();
    void tempRelease();
    void tempUp();

};

#endif // WORKTHREAD_H
