#include "ProductQueryOperationProxy.h"

#include "engine/content/ProductInfo.h"
#include "engine/content/ProductArt.h"
#include "services/debug/DebugService.h"

namespace
{
    // Convert ProductInfoData to the QVariant form that JS expects
    QVariantMap productDataToQVariant(const Origin::Services::Publishing::CatalogDefinitionRef &data)
    {
        using namespace Origin::Engine::Content::ProductArt;

        QVariantMap ret;

        ret["contentId"] = data->contentId();
        ret["expansionId"] = data->financeId();
        ret["masterTitleId"] = data->masterTitleId();
        ret["productId"] = data->productId();
        ret["title"] = data->displayName();
        ret["bannerUrls"] = urlsForType(data, OwnedGameDetailsBanner);
        ret["boxartUrls"] = urlsForType(data, HighResBoxArt) + urlsForType(data, LowResBoxArt);
        ret["thumbnailUrls"] = urlsForType(data, ThumbnailBoxArt);
        ret["localesSupported"] = data->supportedLocales();

        return ret;
    }
}

namespace Origin
{
namespace Client
{
namespace JsInterface
{
    using Engine::Content::ProductInfo;

    ProductQueryOperationProxy::ProductQueryOperationProxy(QObject *parent, ProductInfo *info, QString productId) :
        QObject(parent),
        mProductInfo(info),
        mProductIds(productId),
        mQueriedAsArray(false)
    {
        attachSignals();
    }

    ProductQueryOperationProxy::ProductQueryOperationProxy(QObject *parent, ProductInfo *info, QStringList productIds) :
        QObject(parent),
        mProductInfo(info),
        mProductIds(productIds),
        mQueriedAsArray(true)
    {
        attachSignals();
    }

    void ProductQueryOperationProxy::querySucceeded()
    {
        if (!mQueriedAsArray)
        {
            // Return as a single object
            emit succeeded(productDataToQVariant(mProductInfo->data(mProductIds.first())));
            return;
        }

        QVariantList productInfoList;
        for(QStringList::ConstIterator it = mProductIds.constBegin();
            it != mProductIds.constEnd();
            it++)
        {
            productInfoList << productDataToQVariant(mProductInfo->data(*it));
        }

        emit succeeded(productInfoList);
    }

    void ProductQueryOperationProxy::queryFailed()
    {
        if (!mQueriedAsArray)
        {
            // Return as a single object
            emit failed(mProductIds.first());
            return;
        }

        emit failed(mProductIds);
    }

    void ProductQueryOperationProxy::attachSignals()
    {
        ORIGIN_VERIFY_CONNECT(mProductInfo, SIGNAL(failed()), this, SLOT(queryFailed()));
        ORIGIN_VERIFY_CONNECT(mProductInfo, SIGNAL(succeeded()), this, SLOT(querySucceeded()));

        ORIGIN_VERIFY_CONNECT(mProductInfo, SIGNAL(finished()), mProductInfo, SLOT(deleteLater()));
        ORIGIN_VERIFY_CONNECT(mProductInfo, SIGNAL(finished()), this, SLOT(deleteLater()));
    }
}
}
}
