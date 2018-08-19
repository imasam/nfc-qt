#ifndef SQLITEHELPER_H
#define SQLITEHELPER_H

#include <QSqlQuery>
#include "gps.h"

class SqliteHelper
{
public:
    SqliteHelper();
    // Insert a new card to database
    bool insertCard(const QString& category, const QString& name, const QString& uid, double longtitude, double latitude);
    bool deleteCard(const QString& name);                       // Delete card from database
    QStringList* queryNameList();                               // Query all uid from database
    char* queryUid(const QString& name);                        // Query uid by name
    char* queryCurrentName();                                   // Get the name of current card
    bool setCurrentName(const QString& name);                   // Set the name of current card to database
    QString* queryBusCardName();
    QString* querySubwayCardName();
    QMap<QString, GPS>* queryOthersList();                      // Get the list of cards whose category is others

    /*
     * 查询 在时间为hour时，推荐为recommended但用户选择_final的次数
     * 返回-1表示不存在此纪录
     */
    int queryConflictTime(const QString &recommended, const QString &_final, int hour);
    /*
     * 在时间为hour时，推荐为recommended但用户选择_final的次数加1
     */
    bool increaseConflictTime(const QString &recommended, const QString &_final, int hour);

private:
    bool initTable();

private:
    QSqlQuery query;

};

#endif // SQLITEHELPER_H
