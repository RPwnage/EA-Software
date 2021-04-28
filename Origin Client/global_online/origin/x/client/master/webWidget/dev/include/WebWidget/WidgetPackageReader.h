#ifndef _WEBWIDGET_WIDGETPACKAGEREADER_H
#define _WEBWIDGET_WIDGETPACKAGEREADER_H

#include <QLocale>
#include <QStringList> 

#include "WebWidgetPluginAPI.h"
#include "WidgetPackage.h"
#include "WidgetConfiguration.h"
#include "LocalizedWidgetFile.h"

namespace WebWidget
{
    ///
    /// High level accessor for widget packages
    ///
    /// WidgetPackageReader is the primary interface for almost all functionality related to widget packages. 
    /// This includes:
    ///
    /// - Locating widget package files based on the user agent locale
    /// - Reading the widget's configuration
    /// - Locating the widget's start file
    ///
    /// A WidgetPackageReader can be exposed via the widget:// URI scheme using WidgetNetworkAccessManager
    ///
    class WEBWIDGET_PLUGIN_API WidgetPackageReader
    {
    public:
        ///
        /// Creates a new widget package reader for the given package and locale
        ///
        /// \param  package  Widget package to access the contents of.
        ///                  Any number of WidgetPackageReaders may be associated with a single WidgetPackage.
        /// \param  locale   Locale to read the package in
        ///
        WidgetPackageReader(const WidgetPackage &package, const QLocale &locale);

        ///
        /// Returns the widget package we were created with
        ///
        const WidgetPackage& package() const { return mPackage; }

        ///
        /// Returns the locale we were created with
        ///
        QLocale locale() const { return mLocale; }

        ///
        /// Returns if the widget package we're reading is valid
        ///
        /// Valid widget packages have a valid configuration and the package's start file can be located. Callers
        /// should not attempt to display or interpret an invalid widget package. See
        /// http://www.w3.org/TR/widgets/#invalid-widget-package-0 for the definition of an invalid package. 
        ///
        /// \sa WidgetConfiguration::isValid()
        /// \sa startFile()
        ///
        bool isValidPackage() const;

        ///
        /// Returns the configuration for this widget package
        ///
        const WidgetConfiguration& configuration() const { return mConfiguration; }

        ///
        /// Finds a widget file inside the package
        ///
        /// This implements the rules for finding localized package files defined in
        /// http://www.w3.org/TR/widgets/#rule-for-finding-a-file-within-a-widget-package
        ///
        /// \param  widgetPath        Path of the widget file to search for
        /// \return LocalizedWidgetFile for the file. Use LocalizedWidgetFile::exists() to determine if the find was successful
        ///
        LocalizedWidgetFile findFile(const QString &widgetPath) const;

        ///
        /// Returns the start file for the package
        ///
        /// This implements the rules for finding localized start files defined in
        /// http://www.w3.org/TR/widgets/#step-8-locate-the-start-file
        ///
        /// \return LocalizedWidgetFile for the start file
        ///
        LocalizedWidgetFile startFile() const;

    private:
        QStringList buildSearchLocales() const;
        LocalizedWidgetFile findStartFile() const;
        
        // Used to find a file when we already know the media type and encoding
        LocalizedWidgetFile findFile(const QString &widgetPath, const QByteArray &mediaType, const QByteArray &encoding) const;

        // Our widget configuration
        WidgetConfiguration mConfiguration;
        // Our backend package
        WidgetPackage mPackage;
        // Our start file (may not exist for invalid packages)
        LocalizedWidgetFile mStartFile;

        // The locale we were built with
        QLocale mLocale;
        // The locales we search in reverse order of preference
        // This known as the user agent locales in the standard
        QStringList mSearchLocales;
    };
}

#endif
