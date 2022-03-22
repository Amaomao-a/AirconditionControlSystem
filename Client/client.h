#ifndef CLIENT_H
#define CLIENT_H

#include <QMainWindow>
#include <QTimer>
#include <QWebSocket>          // 通信协议
#include <QJsonDocument>    // 数据包的封装与解析
#include <QJsonObject>
#include <QUuid>                  //用于生成唯一标识符refId和token
#include <QLabel>
#include <QMessageBox>
#include <qmath.h>
#include <QButtonGroup>
#include <QWizard>
#include <QThread>

#include "workthread.h"

#define  INIT_TMP   20.0
#define  ROOM_ID   "304d"

#define  WIND_LOW_LEVEL  0.4
#define  WIND_MID_LEVEL   0.5
#define  WIND_HIGH_LEVEL 0.6

extern ROOM *myRoom, *lastState;     // 前者为房间存储信息，后者用于保存设置的参数，用于回滚？

namespace Ui {
class Client;
}

class Client : public QMainWindow
{
    Q_OBJECT

public:
    explicit Client(QWidget *parent = nullptr);
    ~Client();

public:
    void init_RoomSet();         // 房间空调参数初始化
    void init_SignalAndUI();   // 加载一些界面和按钮设置
    void websocketStart();     // websocket服务开启
    void connectSrv();             // 连接到服务器
    void packet_ClientInit();   // 建立连接后给服务端发初始包
    void clientAckSend();        // 用于响应服务端的确认
    void controlRoom(int power, double tmp, int wind);  // 根据收包设置空调参数

protected slots:
    void autoReport();            // 心跳包--自动上报      
    void client_connected();
    void client_disconnected();
    void msgRcv(const QString& msg); // 简单调控信息接收
    //void client_error(QAbstractSocket::SocketError error);

private slots:
    void on_power_clicked();
    void on_help_clicked();
    void on_set_clicked();
    void on_tempUp_clicked();
    void on_tempDn_clicked();
    void on_windUp_clicked();
    void on_windDn_clicked();
    void on_connect_clicked();
    void on_workMode_clicked(bool checked);

public slots:
    void tempUpdate();
    void tempRebound();
    void tempRelease();
    void buttonStateChange();

signals:
    void systemStart();
    void systemStop();
    void windChange();
    void powerChanged();

private:
    QWizardPage *pageOne(), *pageTwo(); // 帮助手册第一页、第二页
    void packet_Heart();


private:
    Ui::Client *ui;

    // websocket服务相关变量声明
    QTimer* timerReconnect;  // 重连计时器
    QTimer *myTimer;  // 空调状态计时器，每分钟更新一次状态
    QWebSocket *_pWebsocket;
    bool _connected;    // 是否连接到服务端

    // 空调工作变量
    QStringList _strList; // 命令列表
    QLabel *permanent;   // 状态栏标签显示时间

};

#endif // CLIENT_H
