#ifndef _BOXARTPROXY_H
#define _BOXARTPROXY_H

#include <QObject>

#include "engine/content/Entitlement.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

class ORIGIN_PLUGIN_API BoxartProxy : public QObject
{
	Q_OBJECT

public:
	explicit BoxartProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement);

    QVariantMap getCustomBoxArtInfo();

    // Tile coordinates
    Q_INVOKABLE int imageTop(int tileHeight);

    // Tile coordinates
    Q_INVOKABLE int imageLeft(int tileWidth);

    Q_INVOKABLE double imageScale(int tileWidth);

    // Image coordinates
    Q_PROPERTY(int cropCenterX READ cropCenterX)
    int cropCenterX();

    // Image coordinates
    Q_PROPERTY(int cropCenterY READ cropCenterY)
    int cropCenterY();

    // Is the boxart cropped
    Q_PROPERTY(bool croppedBoxart READ croppedBoxart)
    bool croppedBoxart();

    // REturns a data url string if the boxart has been customized
    Q_PROPERTY(QVariant customizedBoxart READ customizedBoxart)
    QVariant customizedBoxart();

    // Box art URL
	Q_PROPERTY(QStringList boxartUrls READ boxartUrls)
	QStringList boxartUrls();

    Q_PROPERTY(int cropWidth READ cropWidth)
    int cropWidth();

    Q_PROPERTY(int cropHeight READ cropHeight)
    int cropHeight();

    // Image coordinates
    Q_PROPERTY(int cropLeft READ cropLeft)
    int cropLeft();

    // Image coordinates
    Q_PROPERTY(int cropTop READ cropTop)
    int cropTop();

private:

	Origin::Engine::Content::EntitlementRef mEntitlement;

};

}
}
}

#endif