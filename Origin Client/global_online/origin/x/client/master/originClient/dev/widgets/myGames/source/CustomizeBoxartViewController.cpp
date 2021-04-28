
//    Copyright © 2012 Electronic Arts
//    All rights reserved.

#include "CustomizeBoxartViewController.h"
#include "flows/CustomizeBoxartFlow.h"

#include "services/debug/DebugService.h"

#include <QFileDialog>

namespace Origin
{
    namespace Client
    {
        CustomizeBoxartViewController::CustomizeBoxartViewController(CustomizeBoxartFlow* parent)
            : QObject(parent)
            , mParent(parent)
            , mCustomizeBoxartView(NULL)
        {
 
        }

        CustomizeBoxartViewController::~CustomizeBoxartViewController()
        {

        }
            
        void CustomizeBoxartViewController::initialize()
        {
            mCustomizeBoxartView = new CustomizeBoxartView();
            mCustomizeBoxartView->initialize();

            ORIGIN_VERIFY_CONNECT(mCustomizeBoxartView, SIGNAL(setBoxart(const Services::Entitlements::Parser::BoxartData&)), this,
                SIGNAL(boxartChanged(const Services::Entitlements::Parser::BoxartData&)));
            ORIGIN_VERIFY_CONNECT(mCustomizeBoxartView, SIGNAL(removeBoxart()), this, SIGNAL(removeBoxart()));

            ORIGIN_VERIFY_CONNECT(mCustomizeBoxartView, SIGNAL(cancel()), mParent, SLOT(stop()));
        }

        void CustomizeBoxartViewController::focus()
        {
            mCustomizeBoxartView->focus();
        }

        void CustomizeBoxartViewController::onShowCustomizeBoxartDialog(Engine::Content::EntitlementRef game)
        {
            if (!game.isNull())
            {
                mCustomizeBoxartView->showCustomizeBoxartDialog(game);
            }
        }
    }
}
