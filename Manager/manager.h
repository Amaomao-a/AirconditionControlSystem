#ifndef MANAGER_H
#define MANAGER_H

#include <QMainWindow>
#include <QTimer>
#include <QWebSocket>
#include <QUrl>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QUuid>
#include <QMessageBox>

#include <QStackedWidget>
#include <QWizard>
#include <QStringListModel>
#include <QStandardItemModel>

namespace Ui {
class Manager;
}

typedef struct MANAGER{
    // 操作权限
    bool confirm;
    QString refId;  // 发包时的唯一标识符
    QString token; // 标识前台管理员
    QString name;
    QString role;
} reception;

class Manager : public QMainWindow
{
    Q_OBJECT

public:
    explicit Manager(QWidget *parent = nullptr);
    ~Manager();

public:
    QString encryptMD5(QString message);  // 登录时需使用的MD5加密
    void login(const QString name, const QString passwd); //登录包发送
    void logout(const QString name); // 退出包发送
    void getRoomList();

    void openRoom(QString roomId, float defaultTmp);
    void seeRoomInfo(QString roomId);
    void getRoomInfo();
    void closeRoom(QString roomId);
    void simpleCost(QString roomId);
    void detailCost(QString roomId);
    //void detailCostFile();
    void requestReport();

    void packet_schedule(const QString message);
    void msgParse(const QString message);  // 全局包接收、解析、调度并发出信号
    void msgPackage(const QString order);   // 全局包封装

    void websocketStart();      // 开启服务
    void connectSrv();
    void SignalAndUiSet();

    void tableSet();
    void addItemToRoom(QStandardItemModel *model, QString roomID, int power, int workWind, double indoorTemp, double setTemp);
    void addItemToLog(QStandardItemModel *model, QString roomID, int wind, int beginTime, int endTime, double money, double sum);
    void tableUpdate();                               // 房间信息更新
    void tableHeader();
    bool itemIsDuplicate(QStandardItemModel *model, QString roomID);  // 检查该房间是否可用
    void removeItemFromTable(QStandardItemModel *model, QString roomID);  // 退房时从表中删除该房间信息

protected slots:
    void manager_connected();
    void manager_disconnected();

    void logConfirmed();

private slots:
    void on_pushButton_login_clicked();

    void on_openRoom_clicked();
    void on_closeRoom_clicked();
    void on_logout_clicked();
    void on_simpleCost_clicked();
    void on_detailCost_clicked();
    void on_getRoomList_clicked();
    void on_seeRoomInfo_clicked();
    void on_pushButton_clicked();

    void on_scheduleControl_clicked();

    void on_connect_clicked();

    void on_roomInfoTable_doubleClicked(const QModelIndex &index);

signals:
    void serverConfirmed();

private:
    Ui::Manager *ui;

    QTimer *timerReconnect;    // 重连计时器
    QWebSocket* _pWebsocket;
    bool _connected;    // 是否连接中控

    QStringList _strList; // 命令列表
    reception *admin;   // 管理员实例
    QLabel *permanent;   // 状态栏标签显示时间
    QTimer *dateTimer;
    QStringListModel _model, _modelMsg;
    QStringList _roomList, _roomMsg;

    QStandardItemModel *roomListTable, *detailCostTable;
};

#endif // MANAGER_H
