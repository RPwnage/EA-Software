#include "BoxartProxy.h"

#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ProductArt.h"

#include "services/entitlements/BoxartData.h"
#include "services/debug/DebugService.h"


namespace Origin
{
namespace Client
{
namespace JsInterface
{

BoxartProxy::BoxartProxy(QObject *parent, Origin::Engine::Content::EntitlementRef entitlement) 
	: QObject(parent), 
	mEntitlement(entitlement)
{	

}

int BoxartProxy::imageTop(int tileHeight)
{
    int top = 0;
    if (cropWidth() != 0)
    {
        Services::Entitlements::Parser::BoxartData boxartData;
        Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
        if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
        {
            QPixmap pixmap;
            if (pixmap.load(boxartData.getBoxartSource()))
            {
                int imgHeight = pixmap.height();
                int cropHeight = boxartData.getBoxartDisplayArea().height();
                int cropTop = boxartData.getBoxartDisplayArea().top();

                int cropCenterY = cropTop + static_cast<int>(cropHeight / 2.0 + 0.5);
                int imgCenterY = static_cast<int>(imgHeight / 2.0 + 0.5);

                int centeredTop = static_cast<int>((tileHeight - imgHeight) / 2.0 + 0.5);

                top = centeredTop - (cropCenterY - imgCenterY);
            }
        }
    }

    return top;
}

int BoxartProxy::imageLeft(int tileWidth)
{
    int left = 0;
    if (cropWidth() != 0)
    {
        Services::Entitlements::Parser::BoxartData boxartData;
        Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
        if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
        {
            QPixmap pixmap;
            if (pixmap.load(boxartData.getBoxartSource()))
            {
                int imgWidth = pixmap.width();
                int cropWidth = boxartData.getBoxartDisplayArea().width();
                int cropLeft = boxartData.getBoxartDisplayArea().left();

                int cropCenterX = cropLeft + static_cast<int>(cropWidth / 2.0 + 0.5);
                int imgCenterX = static_cast<int>(imgWidth / 2.0 + 0.5);

                int centeredLeft = static_cast<int>((tileWidth - imgWidth) / 2.0 + 0.5);

                left = centeredLeft - (cropCenterX - imgCenterX);
            }
        }
    }

    return left;
}

QVariant BoxartProxy::customizedBoxart()
{
    Services::Entitlements::Parser::BoxartData boxartData;
    Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
    if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
    {
        QFile imageFile(boxartData.getBoxartSource());
        QString url;
        if (imageFile.open(QIODevice::ReadOnly))
        {
            url = "data:image/png;base64," + imageFile.readAll().toBase64();
        }

        return url;
    }

    return QVariant();
}

QStringList BoxartProxy::boxartUrls()
{
    QStringList urls;
    QVariant customized = customizedBoxart();

    // Has the user customized boxart for this entitlment
    if (customized.isNull())
    {
        // Something is wrong. NOGs should always have customized boxart.
        if (mEntitlement->contentConfiguration()->itemSubType() == Services::Publishing::ItemSubTypeNonOrigin)
        {
            // Create a default boxart for NOGs
            QString boxartFilename = Engine::Content::CustomBoxartController::createDefaultBoxart(mEntitlement);
            QFile boxartFile(boxartFilename);
            if (boxartFile.open(QIODevice::ReadOnly))
            {
                QString url = "data:image/png;base64," + boxartFile.readAll().toBase64();
                urls << url;
                boxartFile.close();
            }
        }
        else
        {
            // Get the boxart from the network for normal entitlements
            urls << Engine::Content::ProductArt::urlsForType(mEntitlement->contentConfiguration(), Engine::Content::ProductArt::HighResBoxArt);
            urls << Engine::Content::ProductArt::urlsForType(mEntitlement->contentConfiguration(), Engine::Content::ProductArt::LowResBoxArt);
        }
    }
    else
    {
        urls << customized.toString();
    }

    return urls;
}

bool BoxartProxy::croppedBoxart()
{
    return cropWidth() != 0;
}

int BoxartProxy::cropCenterY()
{
    return cropTop() + static_cast<int>(static_cast<double>(cropHeight()) / 2.0 + 0.5);
}

int BoxartProxy::cropCenterX()
{
    return cropLeft() + static_cast<int>(static_cast<double>(cropWidth()) / 2.0 + 0.5);
}

int BoxartProxy::cropWidth()
{
    Services::Entitlements::Parser::BoxartData boxartData;
    Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
    if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
    {
        return boxartData.getBoxartDisplayArea().width();
    }

    return 0;
}

int BoxartProxy::cropHeight()
{
    Services::Entitlements::Parser::BoxartData boxartData;
    Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
    if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
    {
        return boxartData.getBoxartDisplayArea().height();
    }

    return 0;
}

int BoxartProxy::cropLeft()
{
    Services::Entitlements::Parser::BoxartData boxartData;
    Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
    if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
    {
        return boxartData.getBoxartDisplayArea().left();
    }

    return 0;
}

int BoxartProxy::cropTop()
{
    Services::Entitlements::Parser::BoxartData boxartData;
    Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();
    if (c && c->boxartController()->getBoxartData(mEntitlement->contentConfiguration()->productId(), boxartData))
    {
        return boxartData.getBoxartDisplayArea().top();
    }

    return 0;
}

double BoxartProxy::imageScale(int tileWidth)
{
    double scale = 1.0;
    int width = cropWidth();
    if (width != 0 && tileWidth != 0)
    {
        scale = static_cast<double>(tileWidth) / static_cast<double>(width);
    }

    return scale;
}

QVariantMap BoxartProxy::getCustomBoxArtInfo()
{
    QVariantMap obj;
    obj["customizedBoxart"] = customizedBoxart().toUrl().toString();
    obj["cropCenterX"] = cropCenterX();
    obj["cropCenterY"] = cropCenterY();
    obj["croppedBoxart"] = croppedBoxart();
    obj["cropWidth"] = cropWidth();
    obj["cropHeight"] = cropHeight();
    obj["cropLeft"] = cropLeft();
    obj["cropTop"] = cropTop();

    return obj;
}

}
}
}