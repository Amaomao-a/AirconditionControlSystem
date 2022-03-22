#include "client.h"
#include "ui_client.h"
ROOM *myRoom, *lastState;     // 前者为房间存储信息，后者用于保存设置的参数，用于回滚？

Client::Client(QWidget *parent) :
    QMainWindow(parent), ui(new Ui::Client),
    _pWebsocket(nullptr), _connected(false)
{
    ui->setupUi(this);
    ui->stackedWidget->setCurrentIndex(0);

    init_RoomSet();      // 对房间空调进行默认参数设置

    init_SignalAndUI(); // 设置信号与界面
}


Client::~Client()
{
    delete ui;
    if(_pWebsocket)
    {
        _pWebsocket->deleteLater();
        _pWebsocket = nullptr;
    }
}  

void Client::init_RoomSet()
{
    // 初始化房间及空调参数
    myRoom = new ROOM;

    myRoom->roomID = ROOM_ID;
    myRoom->mode=0;
    myRoom->power=0;
    myRoom->applyWind=0;
    myRoom->setTmp = 20;
    myRoom->indoorTmp = 25;
    myRoom->initTmp = 25;
    myRoom->ipAddress="10.128.206.5";
    myRoom->port="7777";

    // 命令列表初始化
    _strList << "/server/openRoom"
                << "/server/closeRoom"
                << "/server/change"
                << "/server/update"
                << "/server/tempRelease"
                << "/server/confirm"
                << "/server/error";
}

void Client::init_SignalAndUI()
{
    ui->workMode->setCheckState(Qt::Checked);

    // 初始化按钮状态
    ui->setTemp->display(QString("%1").arg(INIT_TMP));
//    ui->power->setEnabled(false);
//    ui->set->setEnabled(false);
//    ui->tempUp->setEnabled(false);
//    ui->tempDn->setEnabled(false);
//    ui->windUp->setEnabled(false);
//    ui->windDn->setEnabled(false);
//    ui->workMode->setEnabled(false);

    // 状态栏时间标签
    permanent = new QLabel(this);
    permanent->setFrameStyle(QFrame::Box | QFrame::Sunken);
    permanent->setText(QDateTime::currentDateTime().toString(tr("yyyy-MM-dd dddd hh:mm:ss")));
    ui->statusBar->addPermanentWidget(permanent);

    myTimer = new QTimer(this);
    connect(myTimer, SIGNAL(timeout()), this, SLOT(autoReport()));

    ui->connect->setFocus();
    ui->connect->setDefault(true);

    connect(this, SIGNAL(powerChanged()), this, SLOT(buttonStateChange()));
}

void Client::websocketStart()
{
    // 开启websocket服务
    _pWebsocket = new QWebSocket();
    connect(_pWebsocket, &QWebSocket::connected, this, &Client::client_connected);
    connect(_pWebsocket, &QWebSocket::disconnected, this, &Client::client_disconnected);
    connect(_pWebsocket, &QWebSocket::textMessageReceived, this, &Client::msgRcv);
    //connect(_pWebsocket, &QWebSocket::error, this, &Client::client_error);

    // websocket重连计时
    timerReconnect = new QTimer(this);
    connect(timerReconnect, &QTimer::timeout, [=](){
        qDebug() << "try to connecting...";
        if(!_connected) connectSrv();
        timerReconnect->start(3000);
    });
}

void Client::connectSrv()
{
    QString path = QString("ws://%1:%2").arg(myRoom->ipAddress, myRoom->port);
    QUrl url = QUrl(path);

    _pWebsocket->open(url);
}

void Client::client_connected()
{
    _connected = true;
    timerReconnect->stop();
    QMessageBox::information(nullptr, "连接参数设置", "已成功连接中央空调!", QMessageBox::Ok);
    ui->stackedWidget->setCurrentIndex(1);
    ui->power->setEnabled(true);

    packet_ClientInit();
}

void Client::client_disconnected()
{
    _connected = false;
    timerReconnect->start(1);
    qDebug() << "disconnected";

    ui->power->setEnabled(false);
    ui->set->setEnabled(false);
    ui->tempUp->setEnabled(false);
    ui->tempDn->setEnabled(false);
    ui->windUp->setEnabled(false);
    ui->windDn->setEnabled(false);
}

void Client::packet_ClientInit()
{
    QJsonObject json, airdata;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/client/init";

    airdata["roomId"] = myRoom->roomID;
    airdata["initTmp"] = myRoom->indoorTmp;
    json["data"] = airdata;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Client::autoReport()
{
    // 更新本地时间
    permanent->setText(QDateTime::currentDateTime().toString(tr("yyyy-MM-dd dddd hh:mm:ss")));

    // 发送心跳包
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/client/requestState";
    json["token"] = myRoom->token;

    data["currentTmp"] = QString::number(myRoom->indoorTmp, 'f', 1).toDouble();
    data["roomId"] = myRoom->roomID;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Client::clientAckSend()
{
    QJsonObject json;
    json["refId"] = myRoom->refId;
    json["handler"] = "/client/confirm";
    json["token"] = myRoom->token;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Client::msgRcv(const QString& msg)
{
    QJsonDocument jsonDoc = QJsonDocument::fromJson(msg.toLocal8Bit().data());
    QJsonObject json = jsonDoc.object(), data;  // json用于外层解析，data用于包中包的二次解析

    QString handler = json["handler"].toString();

    switch (_strList.indexOf(handler)){
        case 0:  // "/server/openRoom"
            myRoom->refId = json["refId"].toString();
            data = json["data"].toObject();

            myRoom->token = data["token"].toString();
            myRoom->tempRange = qMakePair(data["tempLow"].toDouble(), data["tempHigh"].toDouble());
            myRoom->mode = data["mode"].toInt();
            if(abs(data["defaultTmp"].toDouble()+1) > qPow(10,-6))  myRoom->setTmp = data["defaultTmp"].toDouble();
            myRoom->applyWind = data["defaultWind"].toInt();
            ui->applyWind->display(myRoom->applyWind);
            ui->setTemp->display(myRoom->setTmp);
            if(myRoom->mode!=ui->workMode->isChecked()) ui->workMode->click();

            clientAckSend(); // 返回确认帧

            break;

        case 1: // "/server/closeRoom"
            myRoom->refId = json["refId"].toString();
            data = json["data"].toObject();
            if(json["token"] != myRoom->token || data["roomId"]!=myRoom->roomID)  return;

            myRoom->power = 0;
            break;

        case 2:  // "/server/change"
            myRoom->refId = json["refId"].toString();
            data = json["data"].toObject();

            if(data["power"].toInt() != -1)    emit powerChanged();
            if(data["mode"].toInt() != -1)     myRoom->mode = data["mode"].toInt();
            if(abs(data["tempLow"].toDouble()+1) > qPow(10,-6)) myRoom->tempRange.first = data["tempLow"].toDouble();
            if(abs(data["tempHigh"].toDouble()+1) > qPow(10,-6)) myRoom->tempRange.second = data["tempHigh"].toDouble();
            if(abs(data["tmp"].toDouble()+1) > qPow(10,-6))  myRoom->setTmp = data["tmp"].toDouble();

            if(data["wind"].toInt() != -1)
            {
                myRoom->workWind = data["wind"].toInt();
                emit windChange();
            }
            if(abs(data["rateFee"].toDouble()+1) > qPow(10,-6)) myRoom->rateFee = data["rateFee"].toDouble();

            ui->setTemp->display(myRoom->setTmp);
            ui->workWind->display(myRoom->workWind);
            if(myRoom->mode != ui->workMode->isChecked())   ui->workMode->click();
            break;

        case 3: // "/server/update"
            myRoom->refId = json["refId"].toString();
            myRoom->totalFee = json["data"].toObject()["totalFee"].toDouble();
            myRoom->currentFee = json["data"].toObject()["currentFee"].toDouble();
            ui->totalFee->display(myRoom->totalFee);
            break;

        case 4: // "/server/tempRelease"
            myRoom->refId = json["refId"].toString();
            myRoom->workWind = json["data"].toObject()["wind"].toInt();
            ui->workWind->display(myRoom->workWind);
            break;

        case 5:  // "/server/confirm"
//            if(refId == myRoom->refId && json["token"]==myRoom->token);
//            else{
//                // wait for new PACKET with correct refId
//            }
            break;

        case 6:  // "/server/error"
                ;
        default:
            qDebug() << QString("收到一个无法解析的包 [%1] ").arg(handler);
    }
    qDebug() << msg;
}

void Client::controlRoom(int power, double tmp, int wind)
{
    QJsonObject  json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/client/controlRoom";
    json["token"] = myRoom->token;

    data["roomId"] = myRoom->roomID;
    if(power == myRoom->power)  power = -1;
    data["power"] = power;
    if(abs(tmp-myRoom->setTmp) < qPow(10,-6)) tmp = -1;
    data["tmp"]  =  QString::number(tmp, 'f', 1).toDouble();
    if(wind == myRoom->workWind)   wind = -1;
    data["wind"] = wind;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Client::packet_Heart()
{
    QJsonObject json, airdata;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/client/updateStatus";
    json["token"] = myRoom->token;

    airdata["roomId"] = myRoom->roomID;
    airdata["currentTmp"] = myRoom->indoorTmp;
    airdata["wind"] = myRoom->workWind;
    json["data"] = airdata;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Client::on_tempUp_clicked()
{
    if(ui->setTemp->value() < myRoom->tempRange.second)   ui->setTemp->display(ui->setTemp->value() +1);
}

void Client::on_tempDn_clicked()
{
    if(ui->setTemp->value() > myRoom->tempRange.first)   ui->setTemp->display(ui->setTemp->value() -1);
}

void Client::on_windDn_clicked()
{
    if(ui->applyWind->value() < 3) ui->applyWind->display(ui->applyWind->value() +1);
    ui->applyWind->repaint();
}

void Client::on_windUp_clicked()
{
    if(ui->applyWind->value() > 0) ui->applyWind->display(ui->applyWind->value() -1);
    ui->applyWind->repaint();
}

void Client::on_set_clicked()
{
    controlRoom(static_cast<int>(!ui->power->isChecked()), ui->setTemp->value(), ui->applyWind->value());
}

void Client::on_power_clicked()
{
    if(myRoom->token.isEmpty())
    {
        QMessageBox::information(nullptr, "开机", "请先到前台激活!", QMessageBox::Ok);
        return ;
    }

    controlRoom(!ui->power->isChecked(), -1, ui->applyWind->value());
}

void Client::buttonStateChange()
{
    QString qss = "QPushButton {border-radius: 8px;color: red;padding:12px 24px; text-align: center;" +
                        QString("text-decoration: none;font-size: 16px;font: 10pt %1;margin: 4px 2px;").arg("Sarasa Term SC") +
                        "background-color: %2;color: black;border: 2px solid #f44336;}";

    myRoom->power = 1 - myRoom->power;
    if(myRoom->power)
    {
//        ui->set->setEnabled(true);
//        ui->tempUp->setEnabled(true);
//        ui->tempDn->setEnabled(true);
//        ui->windUp->setEnabled(true);
//        ui->windDn->setEnabled(true);
//        ui->workMode->setEnabled(true);

        ui->power->setStyleSheet(qss.arg("green"));
        ui->power->setText("ON");
        myTimer->start(6 * 1000);
    }
    else {
//        ui->set->setEnabled(false);
//        ui->tempUp->setEnabled(false);
//        ui->tempDn->setEnabled(false);
//        ui->workMode->setEnabled(false);
//        ui->windUp->setEnabled(false);
//        ui->windDn->setEnabled(false);

        ui->power->setStyleSheet(qss.arg("red"));
        ui->power->setText("OFF");
        myTimer->stop();
    }
}

void Client::on_help_clicked()
{
    QWizard wizard(this);
    wizard.setWindowTitle(tr("用户手册"));
    wizard.addPage(pageOne());
    wizard.addPage(pageTwo());
    wizard.exec();
}

QWizardPage* Client::pageOne()
{
    QWizardPage *firstPage = new QWizardPage;
    firstPage->setTitle(tr("空调操作指南"));
    QLabel *text = new QLabel;
    text->setText(tr("开机--设定温度--送风--维持设定温度--关机"));

    QHBoxLayout *firstLayout = new QHBoxLayout;
    firstLayout->addWidget(text);
    firstPage->setLayout(firstLayout);

    firstPage->setButtonText(QWizard::NextButton, "下一页");
    firstPage->setButtonText(QWizard::CancelButton, "退出");
    return firstPage;
}

QWizardPage* Client::pageTwo()
{
    QWizardPage *secondPage = new QWizardPage;
    secondPage->setTitle(tr("计费标准"));
    QLabel *text = new QLabel;
    text->setText(tr("电费1元/度\n低风1度/3分钟\n中风1度/2分钟\n高风1度/1分钟"));

    QHBoxLayout *secondLayout = new QHBoxLayout;
    secondLayout->addWidget(text);
    secondPage->setLayout(secondLayout);

    secondPage->setButtonText(QWizard::FinishButton, "已知悉");
    secondPage->setButtonText(QWizard::CancelButton, "退出");
    return secondPage;
}

void Client::on_connect_clicked()
{
    if(!ui->lineEdit_ip->text().isEmpty())  myRoom->ipAddress = ui->lineEdit_ip->text();
    if(!ui->port->text().isEmpty())            myRoom->port = ui->port->text();
    if(ui->lineEdit_roomId->text().isEmpty())
    {
        QMessageBox::information(nullptr, "连接参数设置", "房间号不能为空", QMessageBox::Ok);
        return;
    }
    else
    {
        myRoom->roomID = ui->lineEdit_roomId->text();
        websocketStart();    // 开启websocket服务
        connectSrv();

//        if(ui->stackedWidget->currentIndex()==0){
//            QMessageBox::information(nullptr, "连接参数设置", QString("连接失败[%1:%2]").arg(myRoom->ipAddress).arg(myRoom->port), QMessageBox::Ok);
//            return;
//        }
    }
}

void Client::on_workMode_clicked(bool checked)
{
    QString qss = "QCheckBox {border-radius: 8px; color: white;padding: 12px 24px;text-align: center;" +
                          QString("text-decoration: none;font-size: 30px;font: 12pt %1;margin: 4px 2px;background-color: black;color: %2;}").arg("Sarasa Term SC");
    if(checked)
    {
        ui->workMode->setText("制热");
        ui->workMode->setStyleSheet(qss.arg("red"));
    }
    else
    {
        ui->workMode->setText("制冷");
        ui->workMode->setStyleSheet(qss.arg("blue"));
    }
    myRoom->mode = checked;
}

void Client::tempUpdate()
{
    ui->indoorTemp->display(myRoom->indoorTmp);
    ui->indoorTemp->repaint();
}

void Client::tempRebound()
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/client/tempUp";
    json["token"] = myRoom->token;

    data["currentTmp"] = QString::number(myRoom->indoorTmp, 'f', 1).toDouble();
    data["roomId"] = myRoom->roomID;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

void Client::tempRelease()
{
    QJsonObject json, data;
    json["refId"] = QString("%1").arg(qrand() * qrand());
    json["handler"] = "/client/tempRelease";
    json["token"] = myRoom->token;

    data["currentTmp"] = QString::number(myRoom->indoorTmp, 'f', 1).toDouble();
    data["temp"] = QString::number(myRoom->setTmp, 'f', 1).toDouble();
    data["wind"] = myRoom->applyWind;
    data["roomId"] = myRoom->roomID;
    json["data"] = data;

    _pWebsocket->sendTextMessage(QString(QJsonDocument(json).toJson()));
}

