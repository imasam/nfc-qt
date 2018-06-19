#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>

class SqliteHelper
{
public:
    SqliteHelper();
    bool insertUid(char uid[9]);        // Insert a new uid to database
    QStringList* queryUidList();        // Query all uid from database

private:
    bool initTable();

private:
    QSqlQuery query;

};

#endif // SQLITEHELPER_H
