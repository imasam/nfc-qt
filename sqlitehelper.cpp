#include "sqlitehelper.h"
#include <QSqlDatabase>

SqliteHelper::SqliteHelper()
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("nfc.db");
    if(!db.open())
    {
        qDebug() << db.lastError();
        qFatal("Failed To Open Database");
    }
    query = QSqlQuery(db);
}

bool SqliteHelper::isTableExist()
{
    return query.executedQuery()
}
