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
    if(!query.exec("CREATE TABLE IF NOT EXISTS  card ("
                      "name VARCHAR(8) NOT NULL UNIQUE,"
                      "uid CHAR(8) NOT NULL);"))
    {
        qDebug()<<query.lastError();
        return false;
    }

    if(!query.exec("CREATE TABLE IF NOT EXISTS  current ("
                      "name VARCHAR(8));"))
    {
        qDebug()<<query.lastError();
        return false;
    }

    if(!query.exec("INSERT INTO current(name) VALUES('None');"))
    {
        qDebug()<<query.lastError();
        return false;
    }

    return true;
}

bool SqliteHelper::insertUid(QString& name, QString& uid)
{
    if(!query.exec("INSERT INTO card(name,uid) VALUES('"+ name + "','" + uid + "');"))
    {
        qDebug()<<query.lastError();
        return false;
    }
    else
        return true;
}

char* SqliteHelper::queryUid(QString &name)
{
    char* uid = new char[20];
    query.exec("select uid from card where name='" + name + "';");
    if(query.next())
    {
        QString str = query.value(0).toString();
        if(str.length() != 8) return nullptr;

        int i;
        for(i=0; i<str.length(); i++)
            uid[i] = str.at(i).toLatin1();
        uid[i] = '\0';

        return uid;
    }

    return nullptr;
}

char* SqliteHelper::queryCurrentName()
{
    char* name = new char[9];
    query.exec("select name from current;");
    if(query.next())
    {
        QString str = query.value(0).toString();

        int i;
        for(i=0; i<str.length(); i++)
            name[i] = str.at(i).toLatin1();
        name[i] = '\0';

        return name;
    }

    return nullptr;
}

bool SqliteHelper::setCurrentName(QString &name)
{
    return query.exec("update current set name='" + name +"';");
}

QStringList* SqliteHelper::queryNameList()
{
    QStringList* uidList = new QStringList();
    query.exec("select name from card;");
    while(query.next())
    {
        uidList->append(query.value(0).toString());
    }

    return uidList;
}
