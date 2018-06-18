#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>

class SqliteHelper
{
public:
    SqliteHelper();

private:
    bool isTableExist();

private:
    QSqlQuery query;

};

#endif // SQLITEHELPER_H
