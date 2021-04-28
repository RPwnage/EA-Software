#ifndef _WEBWIDGET_WIDGETLINK_H
#define _WEBWIDGET_WIDGETLINK_H 

#include "WebWidgetPluginAPI.h"
#include "UriTemplate.h"

namespace WebWidget
{
    ///
    /// Represents a deep link in to a widget with optional parameters
    ///
    /// This is an Origin-specific extension to load specific pages within a widget
    ///
    /// \sa WidgetConfiguration::links()
    ///
    class WEBWIDGET_PLUGIN_API WidgetLink
    {
    public:
        ///
        /// Create a null widget link
        ///
        WidgetLink()
        {
        }
        
        ///
        /// Creates a widget link with the passed URI and substitution variables
        ///
        /// \param  roleUri     Role URI for the link. This URI must appear in the widget configuration.
        /// \param  variables   Values for the substitution variables to use in expanding the link template
        ///
        WidgetLink(const QString &roleUri, const UriTemplate::VariableMap &variables = UriTemplate::VariableMap()) :
            mRoleUri(roleUri),
            mVariables(variables)
        {
        }

        ///
        /// Returns if this widget link is null
        ///
        bool isNull() const
        {
            return mRoleUri.isNull();
        }

        ///
        /// Returns the role URI for this link
        ///
        QString roleUri() const
        {
            return mRoleUri;
        }
        
        ///
        /// Returns the substitution variable values for this link
        ///
        UriTemplate::VariableMap variables() const
        {
            return mVariables;
        }

    private:
        QString mRoleUri;
        UriTemplate::VariableMap mVariables;
    };
}

#endif

