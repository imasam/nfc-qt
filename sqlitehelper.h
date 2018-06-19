#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>

class SqliteHelper
{
public:
    SqliteHelper();
    bool insertUid(char uid[9]);        // 插入一个新的uid
    QStringList* queryUidList();        // 查询uid列表

private:
    bool initTable();

private:
    QSqlQuery query;

};

#endif // SQLITEHELPER_H
