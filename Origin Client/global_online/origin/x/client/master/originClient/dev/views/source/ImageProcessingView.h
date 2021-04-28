/////////////////////////////////////////////////////////////////////////////
// ImageProcessingView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#ifndef IMAGEPROCESSINGVIEW_H
#define IMAGEPROCESSINGVIEW_H

#include "WebWidget/WidgetView.h"
#include "services/plugin/PluginAPI.h"

#include <QPixmap>

namespace Origin
{
	namespace Client
	{
         namespace
        {
            const QString ImageProcessingCropLinkRole("http://widgets.dm.origin.com/linkroles/cropimage");
        }

        class OriginWebController;
        class ImageProcessingJsHelper;

        class ORIGIN_PLUGIN_API ImageProcessingView : public WebWidget::WidgetView
		{
			Q_OBJECT

		    public:
			    ImageProcessingView(const QString& imagePath, QWidget* parent = NULL);
                ImageProcessingView(const QPixmap& pixmap, QWidget* parent = NULL);
			    ~ImageProcessingView();

                int getScaledWidth() const;
                int getScaledHeight() const;
                QRect cropArea() const;

            signals:

                void imageCropped(const QPixmap& croppedImage);
                void processingCanceled();
                void cropImageLoaded();

            private:
                
                ImageProcessingJsHelper* mJsHelper;

                void initialize(QWidget* parent);
		};
	}
}

#endif
