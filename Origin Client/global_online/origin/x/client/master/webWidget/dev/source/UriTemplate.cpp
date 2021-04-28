#include <QStringList>

#include "UriTemplate.h"

namespace
{
    // Takes a character from a string if the string starts with it
    bool takeFirst(QString &string, QChar c)
    {
        if (string.startsWith(c))
        {
            string = string.mid(1);
            return true;
        }

        return false;
    }
}

namespace WebWidget
{
    UriTemplate::UriTemplate(const QString &templateString) :
        mTemplateString(templateString)
    {
    }

    QString UriTemplate::templateString() const
    {
        return mTemplateString;
    }

    QUrl UriTemplate::expand(const VariableMap &variables) const
    {
        QList<ExpressionRegion> regions(findExpressionRegions());
        QString workingTemplate(mTemplateString);

        // This keeps the offset between the working string and original string
        int workingOffset = 0; 

        for(QList<WebWidget::UriTemplate::ExpressionRegion>::const_iterator it = regions.constBegin();
            it != regions.constEnd();
            it++)
        {
            const ExpressionRegion &expRegion = *it;

            // Find the body of the expression
            const QString expressionBody(mTemplateString.mid(expRegion.offset + 1, expRegion.length - 2));

            // Find the replacement
            const QString replacement(evaluateExpression(expressionBody, variables));

            // Replace the part of the working template we just evaluated
            workingTemplate.replace(expRegion.offset + workingOffset, expRegion.length, replacement);

            // Update the working offset
            workingOffset += replacement.length() - expRegion.length;
        }

        // XXX: We should be converting Unicode to pct encoding here
        return QUrl::fromEncoded(workingTemplate.toUtf8(), QUrl::StrictMode);
    }
        
    QList<UriTemplate::ExpressionRegion> UriTemplate::findExpressionRegions() const
    {
        QList<ExpressionRegion> regions;

        // Current offset we're parsing from
        int currentOffset = 0;
        
        // Position of the opening bracket
        int openingBracket;

        // Keep going while we find opening brackets
        while((openingBracket = mTemplateString.indexOf('{', currentOffset)) != -1)
        {
            int closingBracket = mTemplateString.indexOf('}', openingBracket + 1);

            if (closingBracket != -1)
            {
                // Start parsing again after the closing bracket
                currentOffset = closingBracket + 1;

                // Record the region
                regions.append(ExpressionRegion(openingBracket, closingBracket - openingBracket + 1));
            }
            else
            {
                // Ran off the end of the string looking for the closing bracket
                // XXX: Should we warn here?
                break;
            }
        }

        return regions;
    }
        
    QString UriTemplate::evaluateExpression(QString expression, const VariableMap &variables) const
    {
        // Allow reserved characters?
        bool reservedExpansion = false;
        
        // Include the variable name and '=' in the replacement string
        enum
        {
            // Never include the variable name
            NoName,
            // Include the variable name followed by =
            NameWithUnconditionalEquals,
            // Include the variable name.
            // If the value is non-empty append = to the name
            NameWithEqualsIfValueNonEmpty
        } variableNameStyle = NoName;

        // String we prepend if there are variables to add
        QString prependString;
        // Separator we use to connect multiple values
        QString separatorString(",");

        // Reserved expansion means we allow reserved characters
        if (takeFirst(expression, '+'))
        {
            // Reserved expansion
            reservedExpansion = true;
        }
        else if (takeFirst(expression, '#'))
        {
            // Fragment expansion
            reservedExpansion = true;
            prependString = "#";
        }
        else if (takeFirst(expression, '.'))
        {
            // Label expansion
            prependString = ".";
            separatorString = ".";
        }
        else if (takeFirst(expression, '/'))
        {
            // Path segment expansion
            prependString = "/";
            separatorString = "/";
        }
        else if (takeFirst(expression, ';'))
        {
            // Path style parameter
            prependString = ";";
            separatorString = ";";
            variableNameStyle = NameWithEqualsIfValueNonEmpty;
        }
        else if (takeFirst(expression, '?'))
        {
            // Form-style query
            prependString = "?";
            separatorString = "&";
            variableNameStyle = NameWithUnconditionalEquals;
        }
        else if (takeFirst(expression, '&'))
        {
            // Form-style extension
            prependString = "&";
            separatorString = "&";
            variableNameStyle = NameWithUnconditionalEquals;
        }

        // We should have a bare list of variables now
        QStringList exprVariableNames = expression.split(',');

        // Find the variable values
        QStringList replacementValues;
        for(QList<QString>::const_iterator it = exprVariableNames.constBegin();
            it != exprVariableNames.constEnd();
            it++)
        {
            if (variables.contains(*it))
            {
                const QString variableValue(variables[*it]);
                QString replacementValue;
                
                if (variableNameStyle != NoName)
                {
                    // Include the variable name
                    replacementValue = QUrl::toPercentEncoding(*it);

                    if ((variableNameStyle == NameWithUnconditionalEquals) ||
                        !variableValue.isEmpty())
                    {
                        // Add the =
                        replacementValue += "=";
                    }
                }

                if (reservedExpansion)
                {
                    // Reserved expansion allows many reserved characters through
                    replacementValue += QUrl::toPercentEncoding(variableValue, QByteArray(":/?#[]@!$&'()*+,;="));
                }
                else
                {
                    replacementValue += QUrl::toPercentEncoding(variableValue);
                }
                
                replacementValues.append(replacementValue);
            }
        }

        // Make sure we don't include our prefix character if we have no values
        if (replacementValues.isEmpty())
        {
            return "";
        }

        return prependString + replacementValues.join(separatorString);
    }
}
