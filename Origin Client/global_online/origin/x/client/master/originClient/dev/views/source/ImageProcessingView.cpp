/////////////////////////////////////////////////////////////////////////////
// ImageProcessingView.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ImageProcessingView.h"
#include "engine/login/LoginController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "WebWidget/WidgetView.h"
#include "WebWidget/WidgetPage.h"
#include "NativeBehaviorManager.h"
#include "WebWidgetController.h"
#include "OriginWebController.h"
#include "ImageProcessingJsHelper.h"

#include <QWebFrame>

namespace Origin
{
	namespace Client
	{

		ImageProcessingView::ImageProcessingView(const QString& imagePath, QWidget* parent)
			: WebWidget::WidgetView(parent)
		{
            mJsHelper = new ImageProcessingJsHelper(imagePath, page()->mainFrame());
            ORIGIN_VERIFY_CONNECT(mJsHelper, SIGNAL(cropImageLoaded()), this, SIGNAL(cropImageLoaded()));
            initialize(parent);
		}

        ImageProcessingView::ImageProcessingView(const QPixmap& pixmap, QWidget* parent)
            : WebWidget::WidgetView(parent)
        {
            mJsHelper = new ImageProcessingJsHelper(pixmap, page()->mainFrame());
            initialize(parent);
        }

		ImageProcessingView::~ImageProcessingView()
		{
		}

        void ImageProcessingView::initialize(QWidget* parent)
        {
            page()->mainFrame()->addToJavaScriptWindowObject("imageProcessingJsHelper", mJsHelper);

            WebWidgetController::instance()->loadUserSpecificWidget(widgetPage(), "imageprocessing", Engine::LoginController::instance()->currentUser());

            // Act and look more native
            NativeBehaviorManager::applyNativeBehavior(this);

        }

        int ImageProcessingView::getScaledWidth() const
        {
            return mJsHelper->scaledWidth();
        }

        int ImageProcessingView::getScaledHeight() const
        {
            return mJsHelper->scaledHeight();
        }

        QRect ImageProcessingView::cropArea() const
        {
            return mJsHelper->cropArea();
        }
	}
}
