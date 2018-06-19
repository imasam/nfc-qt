#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sqlitehelper.h"
#include "nfchelper.h"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    sqlHelper = new SqliteHelper();
    nfcHelper = new NfcHelper();

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

    delete currentName;
    delete currentUid;
    delete list;
}



MainWindow::~MainWindow()
{
    ui->lstUid->clear();
    delete nfcHelper;
    delete sqlHelper;
    delete ui;
}

void MainWindow::generateCards()
{
    char uid[9];

    QString nameStr, uidStr;

    sprintf(uid, "%08x", 0xaabbccdd);
    nameStr = QString("Card1");
    uidStr = QString(uid);
    sqlHelper->insertUid(nameStr, uidStr);

    sprintf(uid, "%08x", 0x01234567);
    nameStr = QString("Card2");
    uidStr = QString(uid);
    sqlHelper->insertUid(nameStr, uidStr);

    sprintf(uid, "%08x", 0x00112233);
    nameStr = QString("Card3");
    uidStr = QString(uid);
    sqlHelper->insertUid(nameStr, uidStr);
}

void MainWindow::on_btnApply_clicked()
{
    if(ui->lstUid->currentItem() == nullptr)
        return;

    QString name = ui->lstUid->currentItem()->text();
    char* uid = sqlHelper->queryUid(name);

    qDebug()<<name;
    qDebug()<<uid;
    if(uid && nfcHelper->setUid(uid))
    {
        nfcHelper->setCurrentUid(uid);
        sqlHelper->setCurrentName(name);
        ui->lblCurrentCard->setText(name);
    }

    delete uid;
}

void MainWindow::on_btnAdd_clicked()
{
    char uid[10];
    int uidLen;

    if(nfcHelper->getNewCard(uid, uidLen))
        addToCardList(QString(uid));
}

void MainWindow::addToCardList(const QString& name)
{
    QListWidgetItem* item = new QListWidgetItem();
    item->setText(name);
    ui->lstUid->addItem(name);
}
