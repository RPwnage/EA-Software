#include "ProductArt.h"

#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/CustomBoxartController.h"

#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/rest/AtomServiceResponses.h"
#include "services/settings/SettingsManager.h"

#define VERBOSE_SECURE_BOXART_LOGGING 0

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            namespace ProductArt
            {
                namespace
                {
                    QString filenameForType(ProductArtType type)
                    {
                        switch(type)
                        {
                        case OwnedGameDetailsBanner:
                            return "banner_800x487.jpg";
                        case HighResBoxArt:
                            return "boxart_262x372.jpg";
                        case LowResBoxArt:
                            return "boxart_172x240.jpg";
                        case ThumbnailBoxArt:
                            return "boxart_50x71.jpg";
                        default:
                            ORIGIN_ASSERT_MESSAGE(false, "Unexpected product art type");
                            return QString();
                        }
                    }

                    // EC1-style URLs
                    QStringList urlsForType(const QString &productId, const QString &cdnAssetRoot, ProductArtType type)
                    {
                        // Check if we're an overriden thumbnail
                        if (type == ThumbnailBoxArt)
                        {
                            const QString thumbnailOverride = Services::readSetting(Services::SETTING_OverrideThumbnailUrl.name() + "::" + productId);

                            if (!thumbnailOverride.isEmpty())
                            {
                                // Use our override image
                                QFile imageFile(thumbnailOverride);
                                if (imageFile.open(QIODevice::ReadOnly))
                                {
                                    return QStringList("data:image/png;base64," + imageFile.readAll().toBase64());
                                }
                            }
                        }

                        const QString defaultLocale("en_US");
                        QStringList urls;
                        QString currentLocale(QLocale().name());

                        if (cdnAssetRoot.isEmpty())
                        {
                            // Nothing configured
                            return urls;
                        }

                        // Server is expecting no_NO for norwegian
                        if (currentLocale == "nb_NO")
                        {
                            currentLocale = "no_NO";
                        }

                        // Prefer the localized version
                        // OGD banners currently aren't localized
                        if ((defaultLocale != currentLocale) && (type != OwnedGameDetailsBanner))
                        {
                            urls << cdnAssetRoot + "/images/" + currentLocale + "/" + filenameForType(type);
                        }

                        // Fall back to the default locale
                        urls << cdnAssetRoot + "/images/" + defaultLocale + "/" + filenameForType(type);

                        return urls;
                    }

                    // EC2-style URLs
                    QStringList urlsForType(
                        const QString &productId,
                        ProductArtType imageType,
                        const QString& assetRoot,
                        const QString& packageArtSmall,
                        const QString& packageArtMedium,
                        const QString& packageArtLarge,
                        const QString& backgroundImage,
                        const QString& originSdkImage
                        )
                    {
                        QStringList urls;

                        if (imageType == ThumbnailBoxArt)
                        {
                            // Check if we're an overridden thumbnail
                            const QString thumbnailOverride = Services::readSetting(Services::SETTING_OverrideThumbnailUrl.name() + "::" + productId);

                            if (!thumbnailOverride.isEmpty())
                            {
                                // Use our override image
                                QFile imageFile(thumbnailOverride);
                                if (imageFile.open(QIODevice::ReadOnly))
                                {
                                    return QStringList("data:image/png;base64," + imageFile.readAll().toBase64());
                                }
                            }
                        }

                        QString imagePathRoot(assetRoot);
                        if (imagePathRoot.isEmpty())
                            return urls;

                        QString imagePath;
                        switch (imageType)
                        {
                        case ThumbnailBoxArt:
                            imagePath = packageArtSmall;
                            break;
                        case LowResBoxArt:
                            imagePath = packageArtMedium;
                            break;
                        case HighResBoxArt:
                            imagePath = packageArtLarge;
                            break;
                        case OwnedGameDetailsBanner:
                            imagePath = backgroundImage;
                            break;
                        case SdkArt:
                            imagePath = originSdkImage;
                            break;
                        default:
                            return QStringList();
                            break;
                        }
                        if (imagePath.isEmpty())
                            return urls;

                        QString fullImagePath(imagePathRoot + imagePath);
                        ORIGIN_LOG_DEBUG_IF(VERBOSE_SECURE_BOXART_LOGGING) << "product: " << productId << ", " << fullImagePath;

                        urls << fullImagePath;
                        return urls;
                    }
                }

                QStringList urlsForType(const Services::Publishing::CatalogDefinitionRef &info, ProductArtType type)
                {
                    if (info->imageServer().isEmpty())
                    {
                        return urlsForType(info->productId(), info->cdnAssetRoot(), type);
                    }
                    else
                    {
                        // EC2
                        return urlsForType(
                            info->productId(),
                            type,
                            info->imageServer(),
                            info->packArtSmall(),
                            info->packArtMedium(),
                            info->packArtLarge(),
                            info->backgroundImage(),
                            info->originSdkImage());
                    }
                }

                QStringList urlsForType(const ContentConfigurationRef contentConfig, ProductArtType type)
                {
                    if (contentConfig->imageServer().isEmpty())
                    {
                        // EC1
                        if (contentConfig->isNonOriginGame() && type != OwnedGameDetailsBanner)
                        {
                            // Override using the NOG box art image
                            QString boxartUrl;
                            Services::Entitlements::Parser::BoxartData boxartData;
                            Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();

                            if (c && c->boxartController()->getBoxartData(contentConfig->productId(), boxartData))
                            {
                                QFile imageFile(boxartData.getBoxartSource());
                                if (imageFile.open(QIODevice::ReadOnly))
                                {
                                    boxartUrl = "data:image/png;base64," + imageFile.readAll().toBase64();
                                }
                            }

                            return QStringList(boxartUrl);
                        }

                        return urlsForType(contentConfig->productId(), contentConfig->cdnAssetRoot(), type);
                    }
                    else
                    {
                        // EC2
                        if (contentConfig->isNonOriginGame() && type != OwnedGameDetailsBanner)
                        {
                            // Override using the NOG box art image
                            QString boxartUrl;
                            Services::Entitlements::Parser::BoxartData boxartData;
                            Origin::Engine::Content::ContentController * c = Engine::Content::ContentController::currentUserContentController();

                            if (c && c->boxartController()->getBoxartData(contentConfig->productId(), boxartData))
                            {
                                QFile imageFile(boxartData.getBoxartSource());
                                if (imageFile.open(QIODevice::ReadOnly))
                                {
                                    boxartUrl = "data:image/png;base64," + imageFile.readAll().toBase64();
                                }
                            }

                            return QStringList(boxartUrl);
                        }

                        return urlsForType(
                            contentConfig->productId(),
                            type,
                            contentConfig->imageServer(),
                            contentConfig->packArtSmall(),
                            contentConfig->packArtMedium(),
                            contentConfig->packArtLarge(),
                            contentConfig->backgroundImage(),
                            contentConfig->originSdkImage());
                    }
                }

                QStringList urlsForType(const Services::UserGameData &userGame, ProductArtType type)
                {
                    if (userGame.imageServer.isEmpty())
                    {
                        // EC1
                        if (userGame.cdnAssetRoot.isEmpty())
                        {
                            // EC1 "sometimes" does not contain cdnAssetRoot in the response payload so we cannot calculate it using
                            // urlsForType
                            QStringList myUrls;
                            myUrls.append(userGame.packArtMedium);
                            return myUrls;
                        }
                        else
                        {
                            return urlsForType(userGame.productId, userGame.cdnAssetRoot, type);
                        }
                    }
                    else
                    {
                        // EC2
                        return urlsForType(
                            userGame.productId,
                            type,
                            userGame.imageServer,
                            userGame.packArtSmall,
                            userGame.packArtMedium,
                            userGame.packArtLarge,
                            userGame.backgroundImage,
                            QString());
                    }
                }
            }
        }
    }
}
