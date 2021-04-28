#ifndef _DEFAULTERRORPAGE_H
#define _DEFAULTERRORPAGE_H

#include "AbstractErrorPage.h"
#include "services/plugin/PluginAPI.h"

namespace Origin
{
namespace Client
{

/// \brief Standard Origin web error page. Should be appropriate for most users
class ORIGIN_PLUGIN_API DefaultErrorPage : public AbstractErrorPage
{
public:
    /// \brief Describes the visual alignment of the contents of the error page
    enum Alignment
    {
        /// \brief Indicates left alignment. This is the default
        LeftAlign,
        /// \brief Indicates center alignment
        CenterAlign
    };

    ///
    /// \brief Constructs a DefaultErrorPage with the passed alignmenet
    ///
    /// \param  align  Visual alignment of the error page contents
    ///
    DefaultErrorPage(Alignment align = LeftAlign);
    virtual ~DefaultErrorPage() {}

    static DefaultErrorPage *centerAlignedPage()
    {
        return new DefaultErrorPage(CenterAlign);
    }

    virtual QString offlineErrorHtml() const;
    virtual QString recoveringErrorHtml() const;
    virtual QString pageErrorHtml(PageErrorDetector::ErrorType, QNetworkReply *) const;

    /// \brief Sets the visual alignment of the error page contents
    void setAlignment(Alignment align) { mAlignment = align; }  

    /// \brief Returns the visual alignment of the error page contents
    Alignment alignment() const { return mAlignment; }
    
protected:
    QString buildErrorPage(const QString &title, const QString &body) const;

private:

    Alignment mAlignment;
};

}
}

#endif
