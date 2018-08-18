#include "jsonhelper.h"
#include "gps.h"
#include <QDebug>

bool JsonHelper::init(QString filePath)
{
    QFile file(filePath);
    file.open(QIODevice::ReadOnly);
    QByteArray b = file.readAll();
    file.close();
	QJsonParseError json_error;
	QJsonDocument parse_document = QJsonDocument::fromJson(b, &json_error);
	QJsonObject obj = parse_document.object();
	if (obj.contains("data"))
		data = obj.take("data").toObject();

	return json_error.error == QJsonParseError::NoError;
}

/*
 * ��ȡ��ǰGPS����
 */
GPS* JsonHelper::getCurrentGPS(bool system60)
{
	QJsonObject obj = data;

	QJsonValue value;
	if (system60)
        value = data.take("location_gps_60");
	else
        value = data.take("location_gps_10");

	if (value.isObject())
	{
        GPS* gps = new GPS;
		obj = value.toObject();
		value = obj.take("longitude");
        if (value.isDouble())
            gps->longitude = value.toDouble();
        else if(value.isString())
            gps->longitude = value.toString().toDouble();

		value = obj.take("latitude");
        if (value.isDouble())
            gps->latitude = value.toDouble();
        else if(value.isString())
            gps->latitude = value.toString().toDouble();

        return gps;
	}

    return nullptr;
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

