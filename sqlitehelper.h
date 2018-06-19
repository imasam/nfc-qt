#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>

class SqliteHelper
{
public:
    SqliteHelper();
    bool insertUid(QString& name, QString& uid);    // Insert a new uid to database
    QStringList* queryNameList();                   // Query all uid from database
    char* queryUid(QString& name);                  // Query uid by name
    char* queryCurrentName();                       // Get the name of current card
    bool setCurrentName(QString& name);             // Set the name of current card to database

private:
    bool initTable();

private:
    QSqlQuery query;

};

#endif // SQLITEHELPER_H
