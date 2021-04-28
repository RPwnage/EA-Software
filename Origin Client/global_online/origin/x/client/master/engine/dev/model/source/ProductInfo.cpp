///////////////////////////////////////////////////////////////////////////////
// ProductInfo.cpp
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#include <QHash>

#include "engine/content/ProductInfo.h"
#include "engine/content/ContentConfiguration.h"
#include "engine/content/ContentController.h"
#include "engine/content/CatalogDefinitionController.h"

#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"

using namespace Origin::Services;
using namespace Origin::Services::Publishing;

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            ProductInfo* ProductInfo::byProductId(const QString& productId)
            {
                ProductInfo* info = new ProductInfo(productId);
                info->start();
                return info;
            }
            
            ProductInfo* ProductInfo::byProductIds(const QStringList& productIds)
            {
                ProductInfo* info = new ProductInfo(productIds);
                info->start();
                return info;
            }
            
            ProductInfo::ProductInfo(const QString &id)
            {
                mIds.append(id);
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(finished()), this, SLOT(deleteLater()));
            }
            
            ProductInfo::ProductInfo(const QStringList &ids)
            {
                mIds.append(ids);
                ORIGIN_VERIFY_CONNECT(this, SIGNAL(finished()), this, SLOT(deleteLater()));
            }
            
            ProductInfo::~ProductInfo()
            {
            }
            
            bool ProductInfo::infoExists(const QString& id) const
            {
                // Have we queried this definition before (or own it)?
                return CatalogDefinitionController::instance()->hasCachedCatalogDefinition(id);
            }
            
            void ProductInfo::start()
            {
                mServerQueryIds.clear();

                for(QStringList::ConstIterator it = mIds.begin(); it != mIds.end(); ++it)
                {
                    if(!infoExists(*it))
                    {
                        mServerQueryIds << *it;
                    }
                }

                if (mServerQueryIds.isEmpty())
                {
                    // Invoke the signal instead of emitting, just in case the caller connects after calling this function.
                    // This will cause the signal to be emitted on the next event loop rather than immediately.
                    QMetaObject::invokeMethod(this, "succeeded", Qt::QueuedConnection);
                    QMetaObject::invokeMethod(this, "finished", Qt::QueuedConnection);
                }
                else
                {
                    // Query the server...
                    foreach(const QString& queryProductId, mServerQueryIds)
                    {
                        CatalogDefinitionQuery* query = CatalogDefinitionController::instance()->queryCatalogDefinition(queryProductId);
                        ORIGIN_VERIFY_CONNECT(query, SIGNAL(finished()), this, SLOT(onQueryFinished()));
                    }
                }
            }

            void ProductInfo::onQueryFinished()
            {
                CatalogDefinitionQuery* query = dynamic_cast<CatalogDefinitionQuery*>(sender());
                ORIGIN_ASSERT(query);

                mServerQueryIds.removeAll(query->offerId());

                if(query->definition() == Origin::Services::Publishing::CatalogDefinition::UNDEFINED)
                {
                    mFailedQueryIds << query->offerId();
                }

                if(mServerQueryIds.isEmpty())
                {
                    if(mFailedQueryIds.isEmpty())
                    {
                        emit succeeded();
                    }
                    else
                    {
                        emit failed();
                    }
                    emit finished();
                }

                query->deleteLater();
            }

            QList<Origin::Services::Publishing::CatalogDefinitionRef> ProductInfo::data() const
            {
                QList<Origin::Services::Publishing::CatalogDefinitionRef> list;
                for(QStringList::ConstIterator it = mIds.constBegin(); it != mIds.constEnd(); ++it)
                {
                    list.append(data(*it));
                }
                return list;
            }
                
            Origin::Services::Publishing::CatalogDefinitionRef ProductInfo::data(const QString &id) const
            {
                if (CatalogDefinitionController::instance()->hasCachedCatalogDefinition(id))
                    return CatalogDefinitionController::instance()->getCachedCatalogDefinition(id);

                // If the data is not in the cache, and not available through the contentController,
                // then the product id is invalid (it could not be fullfilled by the product info request).  
                // Create and return a ProductInfoData that will indicate this error in the client for
                // ease of troubleshooting - if we're not in the production environment.
                Origin::Services::Publishing::CatalogDefinitionRef invalidDef = Origin::Services::Publishing::CatalogDefinition::createEmptyDefinition();
                invalidDef->setProductId(id);
                if (Origin::Services::readSetting(Services::SETTING_EnvironmentName).toString().compare(Services::env::production, Qt::CaseInsensitive) != 0)
                {
                    invalidDef->setDisplayName("ERROR: Invalid Product Id [" + id + "].  ProductInfo request failed.");
                }
                return invalidDef;
            }
        }
    }
}

