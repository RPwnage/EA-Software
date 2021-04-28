#ifndef _WEBWIDGET_CUSTOMSTARTFILE_H
#define _WEBWIDGET_CUSTOMSTARTFILE_H

#include <QString>
#include <QByteArray>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    class WidgetConfiguration;

    ///
    /// Represents a custom start file defined by a widget's configuration document
    ///
    class WEBWIDGET_PLUGIN_API CustomStartFile
    {
        friend class WidgetConfiguration;

    public:
        ///
        /// Returns if the custom start file was invalid or unspecified
        ///
        bool isNull() const { return mNull; }

        ///
        /// Returns the widget path to the custom start file
        ///
        QString widgetPath() const { return mWidgetPath; }

        ///
        /// Returns the media type of the custom start file
        ///
        QByteArray mediaType() const { return mMediaType; }

        ///
        /// Returns the character encoding of the custom start file
        ///
        QByteArray encoding() const { return mEncoding; }

    private:
        // Creates a null custom start file
        CustomStartFile() : mNull(true) {}

        CustomStartFile(const QString &widgetPath, const QByteArray &mediaType, const QByteArray &encoding) :
            mNull(false),
            mWidgetPath(widgetPath),
            mMediaType(mediaType),
            mEncoding(encoding)
        {
        }

        bool mNull;

        QString mWidgetPath;
        QByteArray mMediaType;
        QByteArray mEncoding;
    };
}

#endif
