#include "jsonhelper.h"
#include "gps.h"
#include <QDebug>

bool JsonHelper::init(QString filePath)
{
	file = new QFile(filePath);
	file->open(QIODevice::ReadOnly);
	QByteArray b = file->readAll();
	QJsonParseError json_error;
	QJsonDocument parse_document = QJsonDocument::fromJson(b, &json_error);
	QJsonObject obj = parse_document.object();
	if (obj.contains("data"))
		data = obj.take("data").toObject();

	return json_error.error == QJsonParseError::NoError;
}

void JsonHelper::close()
{
	file->close();
}

/*
 * ��ȡ��ǰGPS����
 */
GPS* JsonHelper::getCurrentGPS(bool system60)
{
	GPS* gps = new GPS;
	gps->latitude = 3.1415;
	gps->longitude = 3.1415;
	QJsonObject obj = data;

	QJsonValue value;
	if (system60)
        value = data.take("location_gps_60");
	else
        value = data.take("location_gps_10");

	if (value.isObject())
	{
		obj = value.toObject();
		value = obj.take("longitude");
        if (value.isDouble())
            gps->longitude = value.toDouble();

		value = obj.take("latitude");
        if (value.isDouble())
            gps->latitude = value.toDouble();
	}

	return gps;
}

/*
 * ����վ
 */
bool JsonHelper::existBus()
{
	bool existBus = false;

	QJsonValue value = data.take("busStationCount");
	if (value.toInt())
		existBus = true;

	return existBus;
}

/*
* ����վ
*/
bool JsonHelper::existSubway()
{
	bool existSubway = false;

	QJsonValue value = data.take("metroStationCount");
	if (value.toInt())
		existSubway = true;

	return existSubway;
}

