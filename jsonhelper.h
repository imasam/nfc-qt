#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QByteArray>
#include <QFile>

struct GPS
{
	double longitude;
	double latitude;
};

class JsonHelper
{
public:
	GPS* getCurrentGPS(bool system60 = true);
	bool existBus();
	bool existSubway();
	bool init(QString filePath);
	void close();

private:
	QJsonObject data;
	QFile *file;
};