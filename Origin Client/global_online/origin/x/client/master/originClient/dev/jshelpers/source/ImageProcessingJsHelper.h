/////////////////////////////////////////////////////////////////////////////
// ImageProcessingJsHelper.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#ifndef IMAGEPROCESSINGJSHELPER_H
#define IMAGEPROCESSINGJSHELPER_H

#include <QMovie>
#include <QObject>
#include <QPixmap>
#include <QString>

#include "services/plugin/PluginAPI.h"

namespace Origin
{
    namespace Client
    {
        class ORIGIN_PLUGIN_API ImageProcessingJsHelper : public QObject
        {
            Q_OBJECT

        public:
            ImageProcessingJsHelper(const QString& imageFile, QObject *parent = NULL);
            ImageProcessingJsHelper(const QPixmap& pixmap, QObject *parent = NULL);

            // Last played date
            Q_PROPERTY(int maxHeight READ maxHeight)
            int maxHeight() const;

            Q_PROPERTY(QString image READ image)
            QString image() const;

            Q_PROPERTY(int scaledWidth READ scaledWidth)
            int scaledWidth() const;

            Q_PROPERTY(int scaledHeight READ scaledHeight)
            int scaledHeight() const;

            Q_INVOKABLE void setCroppedRect(int x, int y, int width, int height);
            Q_INVOKABLE void cropImageLoadComplete();

			Q_INVOKABLE double scaledRatio();

            //QPixmap croppedImage() const;
            QRect cropArea() const;


        signals:
            void cropImageLoaded();

        private:
            QRect mCroppedRect;
            QPixmap mImage;
            QSize mScaledSize;
            static const int mMaxHeight = 326;
            static const int mMaxWidth = 389;

            void calculateScaledSize();
        };
    }
}

#endif
