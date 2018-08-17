#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sqlitehelper.h"
#include "nfchelper.h"
#include "jsonhelper.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QProcess>
#include <QTimer>
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
    maxDistance = 100.0;
    process = new QProcess(this);
}

void MainWindow::showEvent(QShowEvent *e)
{
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
    updateTimer->start(120*1000);

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
    sqlHelper->insertCard("others", "Dorm14", "8e4ae505", 114.211437, 30.318437);
    sqlHelper->insertCard("others", "Dorm9", "d565c72d", 114.212, 30.3172);
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
    QString currentName =  ui->lblCurrentCard->getText();
    if(currentName.equal("WaterCard"))
        mfclassic("R", "./watercard.dump");

    QString name = ui->lstCard->currentItem()->text();
    applyNewCard(name);

    // If the selected is waterCard, save the data of waterCard
    if(name.equal("WaterCard"))
        mfclassic("W", "./watercard.dump");

    ui->btnApply->setText("Apply Selected Card");
    ui->btnApply->setEnabled(true);
}

void MainWindow::applyNewCard(const QString& name)
{
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
            qDebug()<<currentGPS->longitude<< ", " <<currentGPS->latitude;
            qDebug()<<cardGPS.longitude<<", "<<cardGPS.latitude;
            qDebug()<<dist;
            qDebug()<<"===============";

            applyNewCard(name);
            return;
        }
    }

    if(jsonHelper->existBus() && busCardName)
    {
        applyNewCard(*busCardName);
        return;
    }

    if(jsonHelper->existSubway() && subwayCardName)
    {
        applyNewCard(*subwayCardName);
        return;
    }
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
