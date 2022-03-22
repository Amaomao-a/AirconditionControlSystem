#include "manager.h"
#include "ui_manager.h"

Manager::Manager(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::Manager),
     _pWebsocket(nullptr), _connected(false)
{
    ui->setupUi(this);

    SignalAndUiSet();
    tableSet();
}

void Manager::SignalAndUiSet()
{
    // 创建标签，设置标签样式并显示信息，然后将其以永久部件的形式添加到状态栏
    permanent = new QLabel(this);
    permanent->setFrameStyle(QFrame::Box | QFrame::Sunken);
    permanent->setText(QDateTime::currentDateTime().toString(tr("yyyy-MM-dd dddd hh:mm:ss")));
    ui->statusBar->addPermanentWidget(permanent);
    dateTimer = new QTimer();  // 设置定时器来更新时间
    connect(dateTimer, &QTimer::timeout, [=](){
        permanent->setText(QDateTime::currentDateTime().toString(tr("yyyy-MM-dd dddd hh:mm:ss")));
    });
    dateTimer->start(1000);

    // 设置一些部件属性
    ui->lineEdit_passwd->setEchoMode(QLineEdit::Password);  // 密文密码
    ui->stackedWidget->setCurrentIndex(0);
    ui->pushButton_login->setEnabled(false);

    connect(ui->power, &QRadioButton::clicked, [=](){
            if(ui->power->text().toStdString() == "ON")
            {
                ui->power->setText("OFF");
                ui->power->setChecked(true);
            }
            else
            {
                ui->power->setText("ON");
                ui->power->setChecked(false);
            }
        });  // 电源按钮的显示文本
    connect(ui->mode, &QRadioButton::clicked, [=](){
            if(ui->mode->text().toStdString() == "COLD")
            {
                ui->mode->setText("HOT");
                ui->mode->setChecked(true);
            }
            else
            {
                ui->mode->setText("COLD");
                ui->mode->setChecked(false);
            }
        });  // 工作模式的显示文本

    admin = new reception;
    // 设回车为登录快捷键
    connect(ui->lineEdit_passwd, SIGNAL(returnPressed()), ui->pushButton_login, SIGNAL(clicked()));
    // 载入命令列表
    _strList << "/server/retRole"
                << "/server/error"
                << "/server/retRoomList"
                << "/server/seeRoomInfo"
                << "/server/retSimpleCost"
                << "/server/detailCost"
                << "/server/retReport"
                << "/server/confirm";
}

Manager::~Manager()
{
    delete ui;
    if(_pWebsocket)
    {
        _pWebsocket->deleteLater();
        _pWebsocket = nullptr;
    }
}

void Manager::websocketStart()
{
    // websocket重连计时
    timerReconnect = new QTimer(this);
    connect(timerReconnect, &QTimer::timeout, [=](){
            qDebug() << "try to connecting...";
            if(!_connected) connectSrv();
            timerReconnect->start(3000);
        });

    // 开启websocket服务
    _pWebsocket = new QWebSocket();
    connect(_pWebsocket, &QWebSocket::connected, this, &Manager::manager_connected);
    connect(_pWebsocket, &QWebSocket::disconnected, this, &Manager::manager_disconnected);
    connect(_pWebsocket, &QWebSocket::textMessageReceived, this, &Manager::msgParse);
    //connect(_pWebsocket, &QWebSocket::error, this, &Manager::manager_error);
}

void Manager::connectSrv()
{
    QString SRV_ADDR="10.206.86.75", SRV_PORT="7777";

    if(!ui->HostAddress->text().isEmpty())  SRV_ADDR = ui->HostAddress->text();
    if(!ui->port->text().isEmpty())              SRV_PORT  = ui->port->text();
    QString path = QString("ws://%1:%2").arg(QString(SRV_ADDR), QString(SRV_PORT));
    QUrl url = QUrl(path);

    _pWebsocket->open(url);
}

void Manager::manager_connected()
{
    _connected = true;
    timerReconnect->stop();
    qDebug() << "connected";
    ui->pushButton_login->setEnabled(true);
}

void Manager::manager_disconnected()
{
    _connected = false;
    timerReconnect->start(1);
    qDebug() << "disconnected";
}

QString Manager::encryptMD5(QString message)
{
    return QCryptographicHash::hash(message.toLatin1(),QCryptographicHash::Md5).toHex();
}

void Manager::login(const QString name, const QString passwd)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/login";

    data["adminId"] = name;
    data["password"] = encryptMD5(passwd);  // MD5加密传输密码
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::logout(const QString name)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/logout";
    json["token"] = admin->token;

    data["adminId"] = name;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::logConfirmed()
{
    disconnect(this, &Manager::serverConfirmed, this, &Manager::logConfirmed);
    ui->stackedWidget->setCurrentIndex(1);

    QMessageBox::information(nullptr, "登录", "登录成功!", QMessageBox::Ok);
    ui->role->setText(admin->role);
    ui->role->repaint();
}

void Manager::getRoomList()
{
    QJsonObject json;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/getRoomList";
    json["token"] = admin->token;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::openRoom(QString roomId, float defaultTmp)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/openRoom";
    json["token"] = admin->token;

    data["roomId"] = roomId;
    data["defaultTmp"] = static_cast<double>(defaultTmp);
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::seeRoomInfo(QString roomId)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/seeRoomInfo";
    json["token"] = admin->token;

    data["roomId"] = roomId;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::simpleCost(QString roomId)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/simpleCost";
    json["token"] = admin->token;

    data["roomId"] = roomId;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::detailCost(QString roomId)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/detailCost";
    json["token"] = admin->token;

    data["roomId"] = roomId;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::closeRoom(QString roomId)
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/closeRoom";
    json["token"] = admin->token;

    data["roomId"] = roomId;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::requestReport()
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/requestReport";
    json["token"] = admin->token;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::msgParse(const QString message)
{
    qDebug() << message;

    QJsonDocument jsonDoc = QJsonDocument::fromJson(message.toLocal8Bit().data());
    QJsonObject json = jsonDoc.object();
    QJsonObject data;
    QJsonArray jsonArray;

    QString refId = json["refId"].toString();
    QString handler = json["handler"].toString();
    switch (_strList.indexOf(handler))
    {
        case 0: //  "/server/retRole"
            data = json["data"].toObject();
            admin->name = ui->lineEdit_name->text();
            admin->role = data["role"].toString();
            admin->token = data["token"].toString();

            emit logConfirmed();
            break;

        case 1: //   "/server/error"
            QMessageBox::information(nullptr, "登录", json["msg"].toString(), QMessageBox::Ok);
            break;

        case 2: //  "/server/retRoomList"
            data = json["data"].toObject();
            jsonArray = data["roomList"].toArray();

            roomListTable->clear();
            tableHeader();
            for(auto it=jsonArray.begin(); it!=jsonArray.end(); it++)
            {
                QString item;
                QString roomId = (*it).toObject()["roomId"].toString();
                item += QString("%1\t").arg(roomId);
                bool idle = (*it).toObject()["idle"].toBool();
                if(idle)
                    item += QString("空闲");
                else
                    item += QString("占用");
                _roomList << item;
                _model.setStringList(_roomList);
            }// end of for

//            json["data"].toObject()["roomList"].toArray()[0].toObject()["idle"].toBool();
//            json["data"].toObject()["roomList"].toArray()[0].toObject()["roomId"].toString();
            break;

        case 3: //   "/server/seeRoomInfo"
            data = json["data"].toObject();
            jsonArray = data["roomInfoList"].toArray();

            roomListTable->clear();
            tableHeader();
            for(auto it=jsonArray.begin(); it!=jsonArray.end(); it++)
            {
                QString item;
                QString roomId = (*it).toObject()["roomId"].toString();
                int power = (*it).toObject()["power"].toInt();
                int wind = (*it).toObject()["wind"].toInt();
                int setTmp = (*it).toObject()["setTmp"].toInt();
                int nowTmp = (*it).toObject()["nowTmp"].toInt();

                addItemToRoom(roomListTable, roomId, power, wind, nowTmp, setTmp);
                qDebug() << roomId<< wind<< power<< wind<< nowTmp<<setTmp;
            }// end of for
            ui->roomInfoTable->setModel(roomListTable);

            break;

        case 4: //  "/server/retSimpleCost"
            data = json["data"].toObject();

            break;

        case 5: //  "/server/detailCost"
            jsonArray = json["data"].toArray();

            detailCostTable->clear();
            tableHeader();
            for(auto it=jsonArray.begin(); it!=jsonArray.end(); it++)
            {
                QString roomId = (*it).toObject()["roomId"].toString();
                int begin_time = (*it).toObject()["beginTime"].toInt();
                int end_time = (*it).toObject()["endTime"].toInt();
                double money = (*it).toObject()["money"].toDouble();
                double sum = (*it).toObject()["sum"].toDouble();
                int wind = (*it).toObject()["wind"].toInt();

                addItemToLog(detailCostTable, roomId, wind, begin_time, end_time, money, sum);
            }// end of for
            ui->roomInfoTable->setModel(detailCostTable);

            break;

        case 6: //  "/server/retReport"
            qDebug() << json["roomReportList"].toString();
            break;
        case 7: //  "/server/confirm"
            qDebug() << "server confirmed!";
            emit serverConfirmed();
            break;

        default:
            qDebug() << "无法解析的包: " <<message;
    }
     return ;
}

void Manager::on_pushButton_login_clicked()
{
    QString name = ui->lineEdit_name->text();
    QString pwd = ui->lineEdit_passwd->text();

    if(name.isEmpty() || pwd.isEmpty())
    {
        QMessageBox::information(nullptr, "登录", "请输入完整的用户名与密码", QMessageBox::Ok);
        return;
    }
    connect(this, &Manager::serverConfirmed, this, &Manager::logConfirmed);

    login(name, pwd);
}

void Manager::on_openRoom_clicked()
{
    QString roomID = ui->roomID->text();
    int initTemp = ui->initTemp->text().toInt();

    if(itemIsDuplicate(roomListTable, roomID))
    {
        QMessageBox::information(nullptr, "开房", QString("房间[%1]已被占用!").arg(roomID), QMessageBox::Ok);
        return;
    }

    addItemToRoom(roomListTable, roomID, 0, 0, 25, 25);
    openRoom(roomID, initTemp);
    QMessageBox::information(nullptr, "开房", QString("房间[%1]已成功开启!").arg(roomID), QMessageBox::Ok);
}

void Manager::on_closeRoom_clicked()
{
    QString roomID = ui->roomID->text();

    if(!itemIsDuplicate(roomListTable, roomID))
    {
        QMessageBox::information(nullptr, "退房", QString("房间[%1]不存在!").arg(roomID), QMessageBox::Ok);
        return;
    }

    removeItemFromTable(roomListTable, roomID);
    closeRoom(roomID);
    QMessageBox::information(nullptr, "退房", QString("房间[%1]已成功关闭!").arg(roomID), QMessageBox::Ok);
}

void Manager::on_logout_clicked()
{
    logout(admin->name);
    ui->stackedWidget->setCurrentIndex(0);

    QMessageBox::information(nullptr, "登出", QString("[%1]已登出!").arg(admin->name), QMessageBox::Ok);
}

void Manager::on_simpleCost_clicked()
{
    simpleCost(ui->roomID->text());
}

void Manager::on_getRoomList_clicked()
{
    getRoomList();
    QMessageBox::information(nullptr, "获取房间列表", "Done!", QMessageBox::Ok);
}

void Manager::on_seeRoomInfo_clicked()
{
    seeRoomInfo(ui->roomID->text());
    QMessageBox::information(nullptr, "房间查看", "Done!", QMessageBox::Ok);
}

void Manager::on_detailCost_clicked()
{
    detailCost(ui->roomID->text());
    QMessageBox::information(nullptr, "详细资费", "Done!", QMessageBox::Ok);
}


void Manager::on_pushButton_clicked()
{
    // 注册
    // ...


    QMessageBox::information(nullptr, "用户注册", "用户注册成功!", QMessageBox::Ok);
    ui->stackedWidget->setCurrentIndex(0);
}

void Manager::tableHeader()
{
    detailCostTable->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("房间号")));
    detailCostTable->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("工作风速")));
    detailCostTable->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("开始时间")));
    detailCostTable->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("结束时间")));
    detailCostTable->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("阶段计费")));
    detailCostTable->setHorizontalHeaderItem(5, new QStandardItem(QObject::tr("总费用")));

    roomListTable->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("房间号")));
    roomListTable->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("空调状态")));
    roomListTable->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("服务风速")));
    roomListTable->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("当前温度")));
    roomListTable->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("设定温度")));
}

void Manager::tableSet()
{
    // 1.房间信息表初始化
    roomListTable = new QStandardItemModel();
    roomListTable->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("房间号")));
    roomListTable->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("空调状态")));
    roomListTable->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("服务风速")));
    roomListTable->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("当前温度")));
    roomListTable->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("设定温度")));

    ui->roomInfoTable->setModel(roomListTable); // 将数据模型table与部件绑定

    // 设置表格列宽值及列宽不可变动
    ui->roomInfoTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->roomInfoTable->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->roomInfoTable->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->roomInfoTable->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->roomInfoTable->horizontalHeader()->setSectionResizeMode(4, QHeaderView::Fixed);

    ui->roomInfoTable->setSelectionBehavior(QAbstractItemView::SelectRows);  // 行选择模式
    ui->roomInfoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);      // 表项不可编辑
    ui->roomInfoTable->setContextMenuPolicy(Qt::CustomContextMenu);           //  传统内容菜单

    // 2.详单记录初始化
    detailCostTable = new QStandardItemModel();
    detailCostTable->setHorizontalHeaderItem(0, new QStandardItem(QObject::tr("房间号")));
    detailCostTable->setHorizontalHeaderItem(1, new QStandardItem(QObject::tr("工作风速")));
    detailCostTable->setHorizontalHeaderItem(2, new QStandardItem(QObject::tr("开始时间")));
    detailCostTable->setHorizontalHeaderItem(3, new QStandardItem(QObject::tr("结束时间")));
    detailCostTable->setHorizontalHeaderItem(4, new QStandardItem(QObject::tr("阶段计费")));
    detailCostTable->setHorizontalHeaderItem(5, new QStandardItem(QObject::tr("总费用")));
}

void Manager::addItemToRoom(QStandardItemModel *model, QString roomID, int power,int workWind, double indoorTemp, double setTemp)
{
    // 动态地向表格部件中添加行
    int row = model->rowCount();

    model->setItem(row, 0, new QStandardItem(roomID));
    model->setItem(row, 1, new QStandardItem(QString::number(power, 'd', 0)));
    model->setItem(row, 2, new QStandardItem(QString::number(workWind, 'd', 0)));
    model->setItem(row, 3, new QStandardItem(QString::number(indoorTemp, 'f', 1)));
    model->setItem(row, 4, new QStandardItem(QString::number(setTemp, 'f', 1)));

    // 设置单元格文本显示颜色
    QString itemColor("brown");
    if(!power)  itemColor = "grey";   // 空调没工作，设置背景显示颜色为灰色
    else if(power && !workWind)  itemColor = "red";  // 空调制热工作，显示为红色。
    else if(power && workWind)  itemColor = "blue"; // 空调制冷工作，显示为蓝色。

    for(int j=0; j<model->columnCount(); j++)
    {
        model->item(row, j)->setForeground(QBrush(QColor(itemColor)));
        model->item(row, j)->setTextAlignment(Qt::AlignCenter);
    }

}

void Manager::addItemToLog(QStandardItemModel *model, QString roomID, int wind, int beginTime,int endTime, double money, double sum){
    // 动态地向表格部件中添加行
    int row = model->rowCount();

    model->setItem(row, 0, new QStandardItem(roomID));
    model->setItem(row, 1, new QStandardItem(QString::number(wind, 'd', 0)));
    model->setItem(row, 2, new QStandardItem(QString::number(beginTime, 'd', 0)));
    model->setItem(row, 3, new QStandardItem(QString::number(endTime, 'd', 0)));
    model->setItem(row, 4, new QStandardItem(QString::number(money, 'f', 1)));
    model->setItem(row, 5, new QStandardItem(QString::number(sum, 'f', 1)));

    for(int j=0; j<model->columnCount(); j++)
    {
        model->item(row, j)->setForeground(QBrush(QColor("grey")));
        model->item(row, j)->setTextAlignment(Qt::AlignCenter);
    }
}

void Manager::tableUpdate()
{

}

void Manager::removeItemFromTable(QStandardItemModel *model, QString roomID)
{
    model->removeRow(model->findItems(roomID, Qt::MatchExactly, 0).at(0)->row());
}

bool Manager::itemIsDuplicate(QStandardItemModel *model, QString roomID)
{
    for(int i=0; i<model->rowCount(); i++)
    {
        if(model->item(i,0)->text() == roomID)  return true;
    }

    return false;
}

void Manager::on_scheduleControl_clicked()
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/manager/scheduleControl";
    json["token"] = admin->token;

    data["size"] = ui->SIZE->text().toInt();
    data["tempLow"] = ui->tempDn->text().toInt();
    data["tempHigh"] = ui->tempUp->text().toInt();
    data["rateFee"] = ui->RATEFEE->text().toDouble();

    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Manager::on_connect_clicked()
{
    websocketStart();  // 开启websocket服务
    connectSrv();        // 连接服务器
}

void Manager::on_roomInfoTable_doubleClicked(const QModelIndex &index)
{

}
