#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>

class SqliteHelper
{
public:
    SqliteHelper();
    bool insertCard(const QString& name, const QString& uid);   // Insert a new card to database
    QStringList* queryNameList();                               // Query all uid from database
    char* queryUid(const QString& name);                        // Query uid by name
    char* queryCurrentName();                                   // Get the name of current card
    bool setCurrentName(const QString& name);                   // Set the name of current card to database

private:
    bool initTable();

private:
    QSqlQuery query;

};

#endif // SQLITEHELPER_H
