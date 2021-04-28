#ifndef _WEBWIDGET_URITEMPLATE
#define _WEBWIDGET_URITEMPLATE

#include <QUrl>
#include <QString>
#include <QMap>
#include <QPair>
#include <QList>

#include "WebWidgetPluginAPI.h"

namespace WebWidget
{
    ///
    /// Helper class for representing and expanding templated URIs
    ///
    /// This targets Level 3 URI template syntax as described by RFC 6570
    ///
    class WEBWIDGET_PLUGIN_API UriTemplate
    {
    public:
        ///
        /// Type definition for URI template variables
        ///
        typedef QMap<QString, QString> VariableMap;
        
        ///
        /// Creates a new URI template using the passed template string
        ///
        /// \param  templateString  Template string conforming to the syntax defined in
        ///                         http://tools.ietf.org/html/draft-gregorio-uritemplate-08
        ///
        UriTemplate(const QString &templateString);

        ///
        /// Creates a null URI template
        ///
        UriTemplate() {}

        ///
        /// Returns true if the URI template is null
        ///
        bool isNull() const { return mTemplateString.isNull(); }

        ///
        /// Returns the unexpanded template string used to construct the UriTemplate instance 
        ///
        QString templateString() const;

        ///
        /// Expands the URI template using the passed variables
        ///
        /// \param  variables Values of the substitution variables to use in the expansion
        /// \return Expanded URI
        ///
        QUrl expand(const VariableMap &variables) const; 

    private:
        ///
        /// Tuple of {offset, length} defining the location of a template expression
        ///
        struct WEBWIDGET_PLUGIN_API ExpressionRegion
        {
            ExpressionRegion(int newOffset, int newLength) :
                offset(newOffset), length(newLength)
            {
            }

            int offset;
            int length;
        };

        ///
        /// Finds all template expresions in the template string
        ///
        QList<ExpressionRegion> findExpressionRegions() const;

        ///
        /// Evaluates a template expression with the passed variables
        ///
        /// \param expression  Body of expression to evaluate without opening and closing brackets
        /// \param variables   Variables to evaluate the expression with
        /// \return Evaluated expression
        ///
        QString evaluateExpression(QString expression, const VariableMap &variables) const;

        QString mTemplateString;
    };
}

#endif
