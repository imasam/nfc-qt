#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>

class SqliteHelper
{
public:
    SqliteHelper();
    // Insert a new card to database
    bool insertCard(const QString& category, const QString& name, const QString& uid, double longtitude, double latitude);
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
