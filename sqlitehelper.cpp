#include "sqlitehelper.h"
#include <QSqlDatabase>
#include <QSqlError>
#include <QDebug>

SqliteHelper::SqliteHelper()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("./nfc.db");
    if(!db.open())
    {
        qDebug() << db.lastError();
        qFatal("Failed To Open Database");
    }
    query = QSqlQuery(db);
    if(!initTable())
    {
        qDebug() << db.lastError();
        qFatal("Failed To Init Table");
    }
}

bool SqliteHelper::initTable()
{
    return query.exec("CREATE TABLE IF NOT EXISTS  card ("
                      "uid CHAR(8) NOT NULL);");
}

bool SqliteHelper::insertUid(char uid[9])
{
    if(!query.exec("INSERT INTO card VALUES('"+ QString(uid) + "');"))
    {
        qDebug()<<query.lastError();
        return false;
    }
    else
        return true;
}

QStringList* SqliteHelper::queryUidList()
{
    QStringList* uidList = new QStringList();
    query.exec("select * from card;");
    while(query.next())
    {
        uidList->append(query.value(0).toString());
    }

    return uidList;
}
