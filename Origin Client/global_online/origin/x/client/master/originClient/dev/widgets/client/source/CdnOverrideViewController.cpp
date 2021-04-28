/////////////////////////////////////////////////////////////////////////////
// CdnOverrideViewController.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "CdnOverrideViewController.h"
#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"
#include "originwindow.h"
#include "originwindowmanager.h"
#include "originmsgbox.h"
#include "originsinglelineedit.h"

namespace Origin
{
    namespace Client
    {
        CdnOverrideViewController::CdnOverrideViewController(QObject *parent /*= NULL*/) 
            : QObject(parent)
            , mCdnOverrideWindow(NULL)
			, mLineEdit(NULL)
        {
        }

        CdnOverrideViewController::~CdnOverrideViewController()
        {
        }
        
        void CdnOverrideViewController::show()
        {
            using namespace Origin::UIToolkit;

            if(!mCdnOverrideWindow)
            {
                mCdnOverrideWindow = new OriginWindow(
                    (OriginWindow::TitlebarItems)(OriginWindow::Icon | OriginWindow::Close),
                    NULL, OriginWindow::MsgBox, QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
                
                mLineEdit = new OriginSingleLineEdit(mCdnOverrideWindow);

                mCdnOverrideWindow->msgBox()->setup(OriginMsgBox::NoIcon,
                    "OVERRIDE CDN",
                    "Any download requests made after updating this value will be redirected to the new CDN.  Note that this value does not persist after app shutdown.",
                    mLineEdit);
                mCdnOverrideWindow->manager()->setupButtonFocus();
    
                ORIGIN_VERIFY_CONNECT(mCdnOverrideWindow, SIGNAL(rejected()), this, SLOT(close()));
                ORIGIN_VERIFY_CONNECT(mCdnOverrideWindow->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(onOverride()));
                ORIGIN_VERIFY_CONNECT(mCdnOverrideWindow->button(QDialogButtonBox::Cancel), SIGNAL(clicked()), mCdnOverrideWindow, SLOT(reject()));
            }

            mCdnOverrideWindow->show();
        }

        void CdnOverrideViewController::onOverride()
        {
            Services::Variant newValue = Services::Variant(mLineEdit->text().trimmed());
            Services::SETTING_CdnOverride.updateDefaultValue(newValue);
            close();
        }

        void CdnOverrideViewController::close()
        {
            mCdnOverrideWindow->close();

            mCdnOverrideWindow = NULL;
            mLineEdit = NULL;
        }
    }
}
