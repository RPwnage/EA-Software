/////////////////////////////////////////////////////////////////////////////
// // MandatoryUpdateViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef MANDATORY_UPDATE_VIEW_CONTROLLER_H
#define MANDATORY_UPDATE_VIEW_CONTROLLER_H

#include <QWidget>
#include "services/rest/UpdateServiceResponse.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit 
    {
        class OriginWindow;
    }

    namespace Client
    {
        class ORIGIN_PLUGIN_API MandatoryUpdateViewController : public QWidget
        {
            Q_OBJECT

        public:
            MandatoryUpdateViewController(QWidget *parent);
            ~MandatoryUpdateViewController();

            void init();

        signals:
            void requestUpdateCheck();

        protected slots:
            /// \brief Slot that is triggered when we have an update available
            /// \param update The update that is available
            void updateAvailable(const Services::AvailableUpdate &update);

            /// \brief Slot that is triggered when we have an update pending
            /// \param update The update that is pending
            void updatePending(const Services::AvailableUpdate &update);

            /// \brief Slot that is triggered when we want to check if there is another update
            void checkForUpdate();

        private:
            QString intervalToString(float seconds);

            Origin::UIToolkit::OriginWindow *mUpdatePendingMessage;
        };
    }
}

#endif //MANDATORY_UPDATE_VIEW_CONTROLLER_H
