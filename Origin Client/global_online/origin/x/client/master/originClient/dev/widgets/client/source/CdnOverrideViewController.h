/////////////////////////////////////////////////////////////////////////////
// CdnOverrideViewController.h
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef CDN_OVERRIDE_VIEW_CONTROLLER_H
#define CDN_OVERRIDE_VIEW_CONTROLLER_H

#include <QObject>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace UIToolkit
    {
        class OriginWindow;
        class OriginSingleLineEdit;
    }

	namespace Client
	{
		class ORIGIN_PLUGIN_API CdnOverrideViewController : public QObject
		{
			Q_OBJECT
		public:
            /// \brief The CdnOverrideViewController constructor.
            /// \param parent This object's parent object, if any.
			CdnOverrideViewController(QObject* parent = NULL);

            /// \brief The CdnOverrideViewController destructor.
            ~CdnOverrideViewController();

            /// \brief Shows the CDN Override dialog.
			void show();

        private slots:
            /// \brief Slot that is triggered when the user changes the CDN override.
            void onOverride();

            /// \brief Closes the CDN Override dialog.
            void close();

		private:
            UIToolkit::OriginWindow *mCdnOverrideWindow; ///< The CDN override dialog.
            UIToolkit::OriginSingleLineEdit* mLineEdit; ///< The line edit.
		};
	}
}

#endif