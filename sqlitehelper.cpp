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
                      "category VACHAR(8) NOT NULL,"
                      "name VARCHAR(20) NOT NULL UNIQUE,"
                      "uid CHAR(8) NOT NULL,"
                      "longitude DOUBLE,"
                      "latitude DOUBLE"
                      ");"))
    {
        qDebug()<<query.lastError();
        qDebug()<<query.lastQuery();
        return false;
    }

    if(!query.exec("CREATE TABLE IF NOT EXISTS  current ("
                      "name VARCHAR(8));"))
    {
        qDebug()<<query.lastError();
        qDebug()<<query.lastQuery();
        return false;
    }

    if(!query.exec("INSERT INTO current(name) VALUES('NONE');"))
    {
        qDebug()<<query.lastError();
        qDebug()<<query.lastQuery();
        return false;
    }

    if(!query.exec("CREATE TABLE IF NOT EXISTS  conflict ("
                        "cardA VARCHAR(8) NOT NULL,
                        "cardB VARCHAR(8) NOT NULL,
                        "chosen VARCHAR(8) NOT NULL,
                        "hour INTEGER NOT NULL,
                        "time INTEGER NOT NULL);"))
    {
        qDebug()<<query.lastError();
        qDebug()<<query.lastQuery();
        return false;
    }

    return true;
}

bool SqliteHelper::insertCard(const QString& category, const QString& name, const QString& uid, double longitude, double latitude)
{
    if(!query.exec("INSERT INTO card(category,name,uid,longitude,latitude) VALUES('"
                   + category + "','" + name + "','" + uid + "'," + QString::number(longitude) + "," + QString::number(latitude) +");"))
    {
        qDebug()<<query.lastError();
        qDebug()<<query.lastQuery();
        return false;
    }
    else
        qDebug()<<query.lastQuery();
        return true;
}

bool SqliteHelper::deleteCard(const QString& name)
{
    if(!query.exec("DELETE FROM card where name='" + name + "';"))
    {
        qDebug()<<query.lastError();
        qDebug()<<query.lastQuery();
        return false;
    }
    else
        qDebug()<<query.lastQuery();
        return true;
}

char* SqliteHelper::queryUid(const QString &name)
{
    query.exec("select uid from card where name='" + name + "';");
    if(query.next())
    {
        QString str = query.value(0).toString();
        if(str.length() != 8) return nullptr;

        char* uid = new char[20];
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
    query.exec("select name from current;");
    if(query.next())
    {
        QString str = query.value(0).toString();

        char* name = new char[9];
        int i;
        for(i=0; i<str.length(); i++)
            name[i] = str.at(i).toLatin1();
        name[i] = '\0';

        return name;
    }

    return nullptr;
}

QString* SqliteHelper::queryBusCardName()
{
    query.exec("select name from card where category='bus';");
    if(query.next())
    {
        QString* name = new QString(query.value(0).toString());

        return name;
    }

    return nullptr;
}

QString* SqliteHelper::querySubwayCardName()
{
    query.exec("select name from card where category='subway';");
    if(query.next())
    {
        QString* name = new QString(query.value(0).toString());

        return name;
    }

    return nullptr;
}

int SqliteHelper::queryConflictTime(const QString &cardA, const QString &cardB,
                                     const QString &chosen, int hour)
{
    query.exec("select time from conflict where "
               "cardA='" + cardA + "',");
    if(query.next())
    {
        QString* name = new QString(query.value(0).toString());

        return name;
    }

    return nullptr;
    return 0;
}

bool SqliteHelper::insertConflict(const QString &cardA, const QString &cardB,
                                  const QString &chosen, int hour)
{

}

bool SqliteHelper::setCurrentName(const QString &name)
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

QMap<QString, GPS>* SqliteHelper::queryOthersList()
{
    QMap<QString, GPS>* othersList = new QMap<QString, GPS>;
    query.exec("select name,longitude,latitude from card where category='others';");

    while(query.next())
    {
        QString name = query.value(0).toString();
        GPS gps;
        gps.longitude = query.value(1).toDouble();
        gps.latitude = query.value(2).toDouble();

        othersList->insert(name, gps);
    }

    return othersList;
}
