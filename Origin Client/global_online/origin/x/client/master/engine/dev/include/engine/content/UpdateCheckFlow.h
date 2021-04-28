///////////////////////////////////////////////////////////////////////////////
// Engine::PlayFlow.h
//
// Copyright (c) 2012 Electronic Arts, Inc. -- All Rights Reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef UPDATECHECKFLOW_H
#define UPDATECHECKFLOW_H

#include <QObject>
#include <QTimer>

#include "engine/content/localcontent.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Engine
    {
        namespace Content
        {
            /// \brief TBD.
            class ORIGIN_PLUGIN_API UpdateCheckFlow : public QObject
            {
                Q_OBJECT

                public:
                    /// \brief The PlayFlow constructor.
                    ///
                    /// \param content TBD.
                    UpdateCheckFlow(Origin::Engine::Content::EntitlementRef content);

                    /// \brief The PlayFlow destructor; releases the resources used by a PlayFlow class instance.
                    ~UpdateCheckFlow();
                    
                    /// \brief TBD.
                    void begin();

                    /// \brief TBD.
                    void cancel();

                signals:
                    void updateAvailable();
                    void noUpdateAvailable();

                private slots:
                        
                    /// \brief Fires when the version update check completes in contentController
                    void onOfferVersionUpdateCheckComplete(QString productId, bool offerUpToDate);

                    /// \brief Fires when the verison update check doesn't complete fast enough
                    void onOfferVersionCheckTimeout();

                private:
                    /// \brief TBD.
                    Origin::Engine::Content::EntitlementRef mContent;
                    
                    bool mOfferUpdateCheckInProgress;
                    QTimer mOfferUpdateCheckTimer;
            };
        }
    }
}

#endif

