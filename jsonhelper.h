#ifndef JSONHELPER_H
#define JSONHELPER_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QFile>

struct GPS;

class JsonHelper
{
public:
	GPS* getCurrentGPS(bool system60 = true);
	bool existBus();
	bool existSubway();
    bool init(QString filePath);
	void startConflictDemoMode();		// 开启冲突演示模式
	void stopConflictDemoMode();		// 关闭冲突演示模式

private:
    QJsonObject data;
	bool isConflictDemoMode = false;	// 冲突演示模式是否开启
};

#endif
