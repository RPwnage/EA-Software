#ifndef ORIGINCHROMEBASE_H_
#define ORIGINCHROMEBASE_H_

#include <QtWidgets/QDialog>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

#include <QMargins>
#include <QWidget>
#include <QFrame>

#include "originbanner.h"

namespace Ui{
    class OriginChromeBase;
}

namespace Origin {
    namespace UIToolkit {
/// \brief Base class that combines QDialog and OriginUIBase for Origin windows
        class UITOOLKIT_PLUGIN_API OriginChromeBase : public QDialog, public OriginUIBase
{
public:
	/// \brief Constructor
	/// \param parent Pointer to the QWidget that is the parent of this dialog
	OriginChromeBase(QWidget* parent);
	/// \brief Destructor
	virtual ~OriginChromeBase();

	/// \brief Sets the content of the window
	/// 
	/// If a non-null widget is passed in, the current content (if any) is removed, the content
	/// is placed as the new content.
	/// 
	/// Note: This takes ownership of the content and deletes it at the appropriate time.
	/// \param content Pointer to the widget that will take up the inside of the dialog
	virtual void setContent(QWidget* content);
	/// \brief Removes the content of the window and deletes it 
	void removeContent();
	/// \brief Removes the content and returns the widget
	/// 
	/// This will remove the widget from the content, set the current content to be empty, and
	/// return the widget.
	/// 
	/// Note: This will give ownership back to the calling function.
	/// \return QWidget* Pointer to the content widget
	QWidget* takeContent();
	/// \brief Gets the content widget
	/// \return Pointer to the QWidget that is the content
	virtual QWidget* const content() const {return mContent;}
	/// \brief Gets the title bar widget
	/// \return QWidget* Pointer to the title bar widget
    QWidget* titleBar() const;
	/// \brief Returns the frame that contains the title bar and the content
	/// 
	/// Note: You should never need to acccess this.
	/// \return QFrame* Pointer to the QFrame that contains the title bar and content
	QFrame* innerFrame() const;

	/// \brief Returns the border widths for that window
	/// 
	/// The border-width value is set in the style sheet but have to be hard code values here 
	/// since there isn't a way of looking up stylesheet values at runtime.
	/// 
	/// Note: Virtual method that must be implemented in inheriting classes.
	virtual const QMargins borderWidths() const = 0;
	/// \brief Sets the left and right content margins on the layout that holds the content widget
	/// 
	/// This will set the margin to be the same number on the left and right and -1 on the top and bottom.
	/// \param marginSize size for the the left and right margin
    void setContentMargin(const int& marginSize);

	/// \brief Sets whether or not the banner will be visible
	/// 
	/// Note: Virtual method that must be implemented in inheriting classes.
	/// \param visible True if visible, false if not
    virtual void setBannerVisible(const bool& visible) = 0;
	/// \brief Gets whether a banner is visible
	/// \return bool True if one is visible, false if not visible or no banner exists.
    bool bannerVisible() const;
    
	/// \brief Sets the text of the banner
	/// 
	/// Sets the text of the banner. If there is no banner, one will be created with default
	/// values, inserted into the layout, and made visible.
	/// \param text QString to be set for banner text
    void setBannerText(const QString& text);
	/// \brief Returns the text of the banner
	/// \return QString Text in the banner. If no banner exists, returns an empty string.
    const QString bannerText() const;

	/// \brief Sets the icon of the banner
	/// 
	/// Sets the icon of the banner. If there is no banner, one will be created with default
	/// values, inserted into the layout, and made visible.
	/// \param iconType The type of icon to set the banner to.
    void setBannerIconType(const OriginBanner::IconType& iconType);
    /// \brief Returns the type of icon in the banner
	/// \return IconType The type of icon in the banner. If no banner exists, returns IconUninitialized
    const OriginBanner::IconType bannerIconType() const;


	/// \brief Sets whether or not the message banner will be visible
	/// 
	/// Note: Virtual method that must be implemented in inheriting classes.
	/// \param visible True if visible, false if not    
	virtual void setMsgBannerVisible(const bool& visible) = 0;
	/// \brief Gets whether a message banner is visible
	/// \return bool True if one is visible, false if not visible or no message banner exists.	
    bool msgBannerVisible() const;

	/// \brief Sets the text of the message banner
	/// 
	/// Sets the text of the message banner. If there is no message banner, one will be created with default
	/// values, inserted into the layout, and made visible.
	/// \param text QString to be set for message banner text    
	void setMsgBannerText(const QString& text);
	/// \brief Returns the text of the message banner
	/// \return QString Text in the message banner. If no banner exists, returns an empty string.
	const QString msgBannerText(void) const;

	/// \brief Sets the icon of the message banner
	/// 
	/// Sets the icon of the message banner. If there is no message banner, one will be created with default
	/// values, inserted into the layout, and made visible.
	/// \param iconType The type of icon to set the message banner to.
	void setMsgBannerIconType(const OriginBanner::IconType& iconType);
	/// \brief Returns the type of icon in the message banner
	/// \return IconType The type of icon in the message banner. If no message banner exists, returns IconUninitialized
    const OriginBanner::IconType msgBannerIconType() const;

protected:
	QDialog* getSelf() {return this;}
	virtual QString& getElementStyleSheet() = 0;
	virtual void prepForStyleReload() = 0;
	/// \brief creates a default banner if none already exists
    void ensureCreatedBanner();
	/// \brief creates a default message banner if none already exists
	/// 
	/// Note: Virtual method that must be implemented in inheriting classes.    
	virtual void ensureCreatedMsgBanner() = 0;
	bool event(QEvent*);
	void paintEvent(QPaintEvent*);

    QWidget* mContent;
	Ui::OriginChromeBase *uiBase;

    // Banner that is used with content. Always appears at the top of a 
    // dialog, just below the title bar. It should always span from edge to edge
    OriginBanner* mBanner;

    // Message banners should be used in situations where information is 
    // presented in addition to the success message
    OriginBanner* mMsgBanner;
};
    }
}
#endif
