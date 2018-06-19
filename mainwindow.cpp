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
    //test();
    QStringList* list = sqlHelper->queryUidList();
    for(int i=0; i<list->length(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem;
        item->setText(list->at(i));
        ui->lstUid->addItem(item);
        qDebug()<<list->at(i);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::test()
{
    char uid[9];
    QStringList* list;

    sprintf(uid, "%08x", 0xaabbccdd);
    sqlHelper->insertUid(uid);
    sprintf(uid, "%08x", 0x01234567);
    sqlHelper->insertUid(uid);
    sprintf(uid, "%08x", 0x00112233);
    sqlHelper->insertUid(uid);
    qDebug()<<QString(uid);


    list = sqlHelper->queryUidList();
    qDebug()<<*list;
}

void MainWindow::on_btnAdd_clicked()
{
    if(!ui->lstUid->selectedItems())
        return;

    QString uidStr = ui->lstUid->currentItem()->text();
    if(uidStr.length() < 8)
        return;

    char uid[9];
    for(int i=0; i<8; i++)
        uid[i] = uidStr.at(i).toLatin1();

    uid[8] = '\0';
}
