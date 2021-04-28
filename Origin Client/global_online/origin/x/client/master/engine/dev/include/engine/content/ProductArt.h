///////////////////////////////////////////////////////////////////////////////
// ProductArt.h
//
// Copyright (c) 2013 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////
#ifndef _PRODUCTBOXART_H
#define _PRODUCTBOXART_H

#include <QList>
#include <QStringList>

#include "engine/content/ContentConfiguration.h"
#include "services/plugin/PluginAPI.h"

namespace Origin 
{
    namespace Services
    {
        struct UserGameData;
    }

    namespace Engine
    {
        namespace Content
        {
            /// \brief Utility functions for querying product box art URLs
            namespace ProductArt
            {
                /// \brief Possible box art types
                enum ProductArtType
                {
                    ///
                    /// 800px by 487px banner image for owned game details
                    ///
                    /// This exists for popular products released after mid-2012
                    ///
                    OwnedGameDetailsBanner, 

                    ///
                    /// \brief 262px by 373px "high res" box art
                    ///
                    /// This exists for products released after mid-2012 and some popular older titles
                    ///
                    HighResBoxArt,

                    ///
                    /// \brief 172px by 240px legacy box art
                    ///
                    /// This should exist for all products
                    ///
                    LowResBoxArt,

                    ///
                    /// \brief 50px by 71px thumbnail box art
                    ///
                    /// This should exist for all products
                    ///
                    ThumbnailBoxArt,

                    ///
                    /// \brief Box art for use by the SDK
                    ///
                    /// This should exist for all products
                    ///
                    SdkArt
                };

                ///
                /// \brief Calculates a list of art URLs for a given product and art type
                ///
                /// \param info   Product information to query box art URLs for
                /// \param type   Type to query
                /// \return List of box art URLs in descending preference. Returned URLs might not exist. If a URL
                ///         doesn't exist the next one should be tried
                ///
                ORIGIN_PLUGIN_API QStringList urlsForType(const Services::Publishing::CatalogDefinitionRef &info, ProductArtType type);
                
                ///
                /// \brief Calculates a list of art URLs for a given user game and art type
                ///
                /// \param userGame   User game to query box art URLs for
                /// \param type   Type to query
                /// \return List of box art URLs in descending preference. Returned URLs might not exist. If a URL
                ///         doesn't exist the next one should be tried
                ///
                ORIGIN_PLUGIN_API QStringList urlsForType(const Services::UserGameData &userGame, ProductArtType type);

                ///
                /// \brief Calculates a list of art URLs for a given content configuration and art type
                ///
                /// \param contentConfig  Content configuration to query box art URLs for
                /// \param type           Type to query
                /// \return List of box art URLs in descending preference. Returned URLs might not exist. If a URL
                ///         doesn't exist the next one should be tried
                ///
                ORIGIN_PLUGIN_API QStringList urlsForType(const ContentConfigurationRef contentConfig, ProductArtType type);
            }
        }
    }
}

#endif

