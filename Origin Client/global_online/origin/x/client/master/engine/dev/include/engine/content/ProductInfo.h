///////////////////////////////////////////////////////////////////////////////
// ProductInfo.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef PRODUCTINFO_H
#define PRODUCTINFO_H

#include <QSet>
#include <QString>

#include "services/publishing/CatalogDefinition.h"
#include "services/plugin/PluginAPI.h"

namespace Origin 
{
    namespace Engine
    {
        namespace Content
        {
            /// \brief Wrapper class used to query generic information about entitlements, even if the user is not currently  
            /// entitled to them.  
            ///   
            /// If the user is entitled to the requested entitlement, this class will return the info we recieved about that  
            /// entitlement at login.  If not, it will query the server, unless it has previously queried the server for the  
            /// requested entitlement this session.
            class ORIGIN_PLUGIN_API ProductInfo : public QObject
            {
                Q_OBJECT
            public:
                /// \brief Looks up info on an entitlement.  
                ///   
                /// Queries the ecommerce server and caches the data if it is not already in the cache.
                /// \param productId The product ID associated with an entitlement.
                static ProductInfo* byProductId(const QString& productId);
                
                /// \brief Looks up info on the given entitlements.  
                ///   
                /// Queries the ecommerce server and caches the data if it is not already in the cache.
                /// \param productIds List of product IDs associated with entitlements to look up.
                static ProductInfo* byProductIds(const QStringList& productIds);
                
                /// \brief Gets the data returned by the server for the requested entitlements.
                /// \return All queried entitlements' data.
                QList<Services::Publishing::CatalogDefinitionRef> data() const;
                
                /// \brief Gets the data returned by the server for the requested entitlement.
                /// \param id The entitlement identifier.
                /// \return The entitlement's data.
                Services::Publishing::CatalogDefinitionRef data(const QString& id) const;

            signals:
                /// \brief Emitted when we retrieve the info, regardless of where we got it.
                void finished();
                
                /// \brief Emitted on success
                void succeeded();
                
                /// \brief Emitted on failure
                void failed();

            private slots:
                /// \brief Connected to the CatalogDefinitionQuery's finished() signal.
                void onQueryFinished();

            private:

                /// \brief The ProductInfo constructor.
                ProductInfo(const QString &id);

                /// \brief The ProductInfo constructor.
                ProductInfo(const QStringList &ids);

                /// \brief The ProductInfo destructor; releases the resources of an instance of the ProductInfo class.
                ~ProductInfo();
                
                /// \brief Creates and sends the request to the server. 
                void start();

                /// \brief Returns whether or not we already have the requested information.
                /// \param id The id to check against.
                /// \return True if we have the info in the cache or library.
                bool infoExists(const QString& id) const;

                /// \brief List of all product IDs we're querying
                QStringList mIds;

                // \brief Set of product IDs we're actively querying the server for
                QStringList mServerQueryIds;

				// \brief Set of product IDs that failed to find any catalog information
				QStringList mFailedQueryIds;
            };
        }        
    }
}

#endif // PRODUCTINFO_H
