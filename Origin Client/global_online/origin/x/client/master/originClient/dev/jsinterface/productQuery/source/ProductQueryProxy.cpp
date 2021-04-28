#include "ProductQueryProxy.h"

#include "engine/content/ProductInfo.h"
#include "engine/content/ContentController.h"

namespace Origin
{
namespace Client
{
namespace JsInterface
{

using Engine::Content::ProductInfo;

QObject* ProductQueryProxy::startQuery(QString productId)
{
    const QString opKey = "single/" + productId;

    QPointer<ProductQueryOperationProxy> existingOperation = mOperations.value(opKey);

    if (existingOperation)
    {
        return existingOperation;
    }
        
    ProductInfo *info = ProductInfo::byProductId(productId);
    ProductQueryOperationProxy *newOp = new ProductQueryOperationProxy(this, info, productId);

    mOperations[opKey] = newOp;
    return newOp;
}

QObject* ProductQueryProxy::startQuery(QStringList productIds)
{
    const QString opKey = "multi/" + productIds.join("/");
    
    QPointer<ProductQueryOperationProxy> existingOperation = mOperations.value(opKey);

    if (existingOperation)
    {
        return existingOperation;
    }
    
    ProductInfo *info = ProductInfo::byProductIds(productIds);
    ProductQueryOperationProxy *newOp = new ProductQueryOperationProxy(this, info, productIds);

    mOperations[opKey] = newOp;
    return newOp;
}

bool ProductQueryProxy::userOwnsProductId(QString productId)
{
    using namespace Engine::Content;
    
    EntitlementRef entRef = ContentController::currentUserContentController()->entitlementByProductId(productId);
    return entRef && entRef->contentConfiguration()->owned();
}

bool ProductQueryProxy::userOwnsMultiplayerId(QString multiplayerId)
{
    using namespace Engine::Content;
    
    EntitlementRef entRef = ContentController::currentUserContentController()->entitlementByMultiplayerId(multiplayerId);
    return entRef && entRef->contentConfiguration()->owned();
}

}
}
}
