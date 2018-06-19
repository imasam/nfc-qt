#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "sqlitehelper.h"
#include "nfchelper.h"
#include <QMessageBox>
#include <QInputDialog>
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
    ui->lstCard->clear();
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
    sqlHelper->insertCard(nameStr, uidStr);

    sprintf(uid, "%08x", 0x01234567);
    nameStr = QString("Card2");
    uidStr = QString(uid);
    sqlHelper->insertCard(nameStr, uidStr);

    sprintf(uid, "%08x", 0x00112233);
    nameStr = QString("Card3");
    uidStr = QString(uid);
    sqlHelper->insertCard(nameStr, uidStr);
}

void MainWindow::on_btnApply_clicked()
{
    if(ui->lstCard->currentItem() == nullptr)
        return;

    // Tip user
    ui->btnApply->setEnabled(false);
    ui->btnApply->setText("Applying, PLS wait..");
    ui->btnApply->repaint();

    QString name = ui->lstCard->currentItem()->text();
    char* uid = sqlHelper->queryUid(name);

    qDebug()<<name; qDebug()<<uid;

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

    ui->btnApply->setText("Apply Selected Card");
    ui->btnApply->setEnabled(true);

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
            addToCardList(name);
            sqlHelper->insertCard(name, QString(uid));
        }
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
