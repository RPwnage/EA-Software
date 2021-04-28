#ifndef _GEOLOCATIONQUERYOPERATIONPROXY_H
#define _GEOLOCATIONQUERYOPERATIONPROXY_H

/**********************************************************************************************************
 * This class is part of Origin's JavaScript bindings and is not intended for use from C++
 *
 * All changes to this class should be reflected in the documentation in jsinterface/doc
 *
 * See http://developer.origin.com/documentation/display/EBI/Working+with+Web+Widgets for more information
 * ********************************************************************************************************/

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>

#include "services/rest/HttpStatusCodes.h"
#include "services/rest/OriginServiceValues.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{

namespace Services
{
    class GeoCountryResponse;
}

namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API GeolocationQueryOperationProxy : public QObject
{
    Q_OBJECT
public: 
    GeolocationQueryOperationProxy(QObject *parent, Services::GeoCountryResponse *resp);

signals:
    void succeeded(QVariant info);
    void failed();

private slots:
    void querySucceeded();
    void queryFailed(Origin::Services::HttpStatusCode httpCode);
    void queryFailed(Origin::Services::restError restError);

private:
    Services::GeoCountryResponse *mGeoCountryResponse;
};

}
}
}

#endif

