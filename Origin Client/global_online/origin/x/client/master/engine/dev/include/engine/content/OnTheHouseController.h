//    Copyright © 2011-2012, Electronic Arts
//    All rights reserved.

#ifndef ONTHEHOUSECONTROLLER_H
#define ONTHEHOUSECONTROLLER_H

#include <QObject>
#include "services/rest/OnTheHousePromoService.h"

namespace Origin 
{
    namespace Engine
    {
        namespace Content
        {
            
            /// \brief TBD.
            class OnTheHouseController : public QObject
            {
                Q_OBJECT
            public:
                static OnTheHouseController* sInstance;

                /// \brief The OnTheHouseController destructor; releases the resources of a OnTheHouseController object.
                virtual ~OnTheHouseController();

                /// \brief Creates a new On The House controller.
                ///
                /// \returns a pointer the current instance of the OnTheHouseController, also creates one if none exist.
                static OnTheHouseController* instance();

				void fetchPromo();
                bool isResponseFinished() const {return mResponseFinished;}
                bool willPromoShow() const {return mWillPromoShow;}
                
            signals:
				// This is the signal other should be listening for to determine if we are going to show the offer or not
			    void succeeded(QVariant info);
				void error();
                void finished();

			private slots:
			    void onResponseSuccess();
				void onResponseError(Origin::Services::HttpStatusCode);
                void onResponseFinish();

			private:
				/// \brief The OnTheHouseController constructor.
                /// \param user TBD.
                explicit OnTheHouseController();
                bool mWillPromoShow;
                // We need to manually keep track if the previous response completed successfully. We delete the response when it is finished.
                bool mResponseFinished;
            }; //OnTheHouseController
        } //Content        
    } //Engine
} //Origin

#endif // ONTHEHOUSECONTROLLER_H
