/////////////////////////////////////////////////////////////////////////////
// ImageProcessingJsHelper.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "ImageProcessingJsHelper.h"

#include "services/log/LogService.h"
#include "services/debug/DebugService.h"

#include <QImage>

namespace Origin
{
    namespace Client
    {
        ImageProcessingJsHelper::ImageProcessingJsHelper(const QString& imageFile, QObject *parent)
        : QObject(parent)
        , mScaledSize(QSize(0,0))
        {
            if (mImage.load(imageFile))
            {
                calculateScaledSize();
            }          
        }

        ImageProcessingJsHelper::ImageProcessingJsHelper(const QPixmap& pixmap, QObject *parent)
            : QObject(parent)
            , mImage(pixmap)
            , mScaledSize(QSize(0,0))
        {
            if (!pixmap.isNull())
            {
                calculateScaledSize();
            }
        }
        
        int ImageProcessingJsHelper::maxHeight() const
        {
            return mMaxHeight;
        }

        QString ImageProcessingJsHelper::image() const
        {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::ReadWrite);
            mImage.save(&buffer, "PNG");
            buffer.reset();

            return "data:image/png;base64," + buffer.readAll().toBase64();
        }

        int ImageProcessingJsHelper::scaledWidth() const
        {
            return mScaledSize.width();
        }

        int ImageProcessingJsHelper::scaledHeight() const
        {
            return mScaledSize.height();
        }

        void ImageProcessingJsHelper::setCroppedRect(int x, int y, int width, int height)
        {
            mCroppedRect = QRect(x, y, width, height);
        }

        QRect ImageProcessingJsHelper::cropArea() const
        {
            double ratio = static_cast<double>(mImage.width()) / static_cast<double>(mScaledSize.width());
            QRect unscaledCropRect(mCroppedRect.x() * ratio, mCroppedRect.y() * ratio, mCroppedRect.width() * ratio, mCroppedRect.height() * ratio);

            return unscaledCropRect;
        }

        void ImageProcessingJsHelper::calculateScaledSize()
        {
            QPixmap scaledImage;
            if (static_cast<double>(mMaxHeight) / mImage.height() < static_cast<double>(mMaxWidth) / mImage.width())
            {
                 scaledImage = mImage.scaledToHeight(mMaxHeight);
            }
            else
            {
                 scaledImage = mImage.scaledToWidth(mMaxWidth);
            }

            mScaledSize = scaledImage.size();
        }

        void ImageProcessingJsHelper::cropImageLoadComplete()
        {
            emit cropImageLoaded();
        }

		double ImageProcessingJsHelper::scaledRatio()
		{
			return (static_cast<double>(mImage.width()) / static_cast<double>(mScaledSize.width()));
		}
    }
}
