#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sqlitehelper.h"
#include "nfchelper.h"
#include "jsonhelper.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QTimer>
#include <QTime>
#include <qmath.h>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sqlHelper = new SqliteHelper();
    nfcHelper = new NfcHelper();
    jsonHelper = new JsonHelper();
    currentGPS = new GPS();
    currentGPS->latitude = 3.1415;
    currentGPS->latitude = 3.1415;
    maxDistance = 50.0;
    process = new QProcess(this);
}

void MainWindow::showEvent(QShowEvent *e)
{
    Q_UNUSED(e);

    QStringList* list = sqlHelper->queryNameList();
    if(list->length() == 0)
    {
        delete list;
        generateCards();
        list = sqlHelper->queryNameList();
    }

    for(int i=0; i<list->length(); i++)
    {
        addToCardList(list->at(i));
    }

    busCardName = sqlHelper->queryBusCardName();
    subwayCardName = sqlHelper->querySubwayCardName();
    othersList = sqlHelper->queryOthersList();

    char* currentName = sqlHelper->queryCurrentName();
    char* currentUid = nullptr;
    if(currentName)
    {
        ui->lblCurrentCard->setText(QString(currentName));
        QString name = QString(currentName);
        currentUid = sqlHelper->queryUid(name);
        if(currentUid)
            nfcHelper->setCurrentUid(currentUid);
    }

    updateTimer = new QTimer(this);
    connect(updateTimer, SIGNAL(timeout()), this, SLOT(updateSlot()));
    updateTimer->start(5*1000);

    QTimer::singleShot(3000, this, SLOT(updateSlot()));

    delete currentName;
    delete currentUid;
    list->clear();
    delete list;
}

MainWindow::~MainWindow()
{
    ui->lstCard->clear();
    othersList->clear();
    updateTimer->stop();
    process->close();
    delete nfcHelper;
    delete sqlHelper;
    delete jsonHelper;
    delete busCardName;
    delete subwayCardName;
    delete othersList;
    delete currentGPS;
    delete updateTimer;
    delete process;

    delete ui;
}

void MainWindow::generateCards()
{
    sqlHelper->insertCard("bus", "WuHanTong", "01234567", 3.1415, 3.1415);
    sqlHelper->insertCard("others", "Dorm14", "8e4ae505", 114.212970,30.314324);
    sqlHelper->insertCard("others", "Dorm9", "d565c72d", 114.213005,30.313409);
    sqlHelper->insertCard("others", "WaterCard", "4b3736df", 3.1415, 3.1415);
}

void MainWindow::on_btnApply_clicked()
{
    if(ui->lstCard->currentItem() == nullptr)
        return;

    // Tip user
    ui->btnApply->setEnabled(false);
    ui->btnApply->setText("Applying, PLS wait..");
    ui->btnApply->repaint();

    // If the current is waterCard, save the data of waterCard
    // and restore the empty card
    QString currentName =  ui->lblCurrentCard->text();
    if(QString::compare(currentName, "WaterCard") == 0)
    {
        nfcHelper->mfclassic("R", "./watercard.dump");
        nfcHelper->mfclassic("W", "./empty.dump");
    }

    QString selected = ui->lstCard->currentItem()->text();
    applyNewCard(selected);

    // If the selected is waterCard, save the data of waterCard
    if(QString::compare(selected, "WaterCard") == 0)
        nfcHelper->mfclassic("W", "./watercard.dump");

    ui->btnApply->setText("Apply");
    ui->btnApply->setEnabled(true);

    // 检测用户是否选择了系统推荐以外的卡
    if(selected != recommended)     // 是，冲突次数+1
    {
        int hour = QTime::currentTime().hour();
        sqlHelper->increaseConflictTime(recommended, selected, hour);
    }
}

void MainWindow::applyNewCard(const QString& name)
{
    if(name == nullptr || name == "") return;

    char* uid = sqlHelper->queryUid(name);

    qDebug()<<"Apply a new card:";
    qDebug()<<" "<<name<<", "<<uid;

    // Set uid of the selected to real card/target
    if(uid)
    {
        if(nfcHelper->setUid(uid))
        {
            nfcHelper->setCurrentUid(uid);
            sqlHelper->setCurrentName(name);
            ui->lblCurrentCard->setText(name);
        }
        else
            QMessageBox::about(this, "Fault", "Cannot write the card.");
    }
    else
        QMessageBox::about(this, "Fault", "The selected card is invaid.");

    delete uid;
}

void MainWindow::on_btnAdd_clicked()
{
    char uid[10];
    int uidLen;

    // Tip user
    ui->btnAdd->setEnabled(false);
    ui->btnAdd->setText("Searching card..");
    ui->btnAdd->repaint();

    // Get a new card which is different from the current
    if(nfcHelper->getNewCard(uid, uidLen))
    {
        bool isOK;

        QString name = QInputDialog::getText(this, "New Card",
                                             "Find a new card, uid: " + QString(uid) +
                                             "\nPlease input the name of the card:",
                                             QLineEdit::Normal, nullptr, &isOK);

        if(isOK)
        {
            QStringList categories;
            categories.append("bus");
            categories.append("subway");
            categories.append("others");
            QString category = QInputDialog::getItem(this, "New Card",
                            "Find a new card, uid: " + QString(uid) +
                            "\nPlease choose the category of the card:",
                            categories, 0, false, &isOK);

            double longitude = 3.1415, latitude = 3.1415;

            if(isOK)
            {
                if(category == "others")
                {
                    // Get the longitude and latitude
                    longitude = currentGPS->longitude;
                    latitude = currentGPS->latitude;
                }
                else if(category == "bus")
                    *busCardName = name;
                else if(category == "subway")
                    *subwayCardName = name;

                addToCardList(name);
                sqlHelper->insertCard("others", name, QString(uid), longitude, latitude);
            }
            else
                QMessageBox::about(this, "Fault", "The category isn't set.");
        }
        else
            QMessageBox::about(this, "Fault", "The name isn't set.");
    }
    else
        QMessageBox::about(this, "Fault", "No card different from current.");

    ui->btnAdd->setText("Add New Card");
    ui->btnAdd->setEnabled(true);
}

void MainWindow::addToCardList(const QString& name)
{
    QListWidgetItem* item = new QListWidgetItem();
    item->setText(name);
    ui->lstCard->addItem(name);
}

void MainWindow::updateSlot()
{
    process->close();
    delete process;
    process = new QProcess(this);
    process->start("python /home/pi/nfc-qt/MapAPI/call_test.py");

    jsonHelper->init("./map_data.json");
    GPS* t = jsonHelper->getCurrentGPS();
    if(t != nullptr)
    {
        delete currentGPS;
        currentGPS = t;
    }

    qDebug()<<currentGPS->longitude<<", "<<currentGPS->latitude;

    QString othersName = "";
    bool existBus = jsonHelper->existBus();
    bool existSubway = jsonHelper->existSubway();

    // Calculate the distances
    QMapIterator<QString, GPS> i(*othersList);
    while(i.hasNext())
    {
        QString name = i.next().key();
        GPS cardGPS = i.value();
        double dist = getDistance(currentGPS, &cardGPS);

        if(dist < maxDistance)
        {
            qDebug()<<"===============";
            qDebug()<<cardGPS.longitude<<", "<<cardGPS.latitude;
            qDebug()<<"Distance: "<<dist;
            qDebug()<<"===============";

            othersName = name;
            break;
        }
    }

    /*
     * 既处于其它种类卡范围内也处于公交站或地铁站范围内时进行卡选择判决
     * 默认推荐其它种类卡
     * 当推荐其它种类卡但用户选择公交卡或地铁卡的次数大于等于阈值时，不推荐其它种类卡
     */
    recommended = othersName;       // 默认推荐其它种类卡
    if(othersName != "" && (existBus || existSubway))
    {
        int hour = QTime::currentTime().hour();
        static const int threshold = 2;

        // 检测冲突次数以决定是否更改推荐
        if(busCardName != nullptr       // 首先检测公交卡是否存在
            && sqlHelper->queryConflictTime(othersName, *busCardName, hour) >= threshold)
            recommended = *busCardName;
        else if(subwayCardName != nullptr
            && sqlHelper->queryConflictTime(othersName, *subwayCardName, hour) >= threshold)
            recommended = *subwayCardName;
    }
    else if(existBus)
        recommended = *busCardName;
    else if(existSubway)
        recommended = *subwayCardName;

    applyNewCard(recommended);          // 设置推荐卡
}

double MainWindow::getDistance(GPS* gps1, GPS* gps2)
{
    double Lat1 = rad(gps1->latitude); // 纬度
    double Lat2 = rad(gps2->latitude);
    double a = Lat1 - Lat2;//两点纬度之差
    double b = rad(gps1->longitude) - rad(gps2->longitude); //经度之差
    //计算两点距离的公式
    double s = 2 * qSin(qSqrt(qPow(qSin(a / 2), 2) + qCos(Lat1) * qCos(Lat2) * qPow(qSin(b / 2), 2)));
    s = s * 6378137.0;//弧长乘地球半径（半径为米）
    s = qRound(s * 10000.0) / 10000.0;//精确距离的数值
    return s;
}

double MainWindow::rad(double d)
{
    return d * M_PI / 180.00; //角度转换成弧度
}

void MainWindow::on_btnDelete_clicked()
{
    if(ui->lstCard->currentItem() == nullptr)
        return;

    QString name = ui->lstCard->currentItem()->text();
    int  result = QMessageBox::question(this, "Delete", "Are you sure to delete" + name, QMessageBox::Ok|QMessageBox::Cancel,
                          QMessageBox::Cancel);
    if(result == QMessageBox::Cancel)       // Cancel is clicked
        return;

    if(!sqlHelper->deleteCard(name))
        QMessageBox::about(this, "Fault", "Fail to delete the selected card.");

    if(subwayCardName == name)
    {
        delete subwayCardName;
        subwayCardName = nullptr;
    }
    if(busCardName == name)
    {
        delete busCardName;
        busCardName = nullptr;
    }
    othersList->remove(name);
    QListWidgetItem* item = ui->lstCard->currentItem();
    ui->lstCard->removeItemWidget(item);
    delete item;
}

void MainWindow::on_btnStartConflictDemoMode_clicked()
{
    jsonHelper->startConflictDemoMode();
}

void MainWindow::on_btnStopConflictDemoMode_clicked()
{
    jsonHelper->stopConflictDemoMode();
}
