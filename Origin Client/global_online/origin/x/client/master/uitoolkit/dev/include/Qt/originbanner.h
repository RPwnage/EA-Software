#ifndef ORIGINBANNER_H_
#define ORIGINBANNER_H_

#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

class Ui_OriginBanner;

namespace Origin {
    namespace UIToolkit {

/// \brief Creates a banner with an icon and text. 
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginBanner">Click here for more OriginBanner documentation</A>
/// 
/// The banner can appear inside of a dialog when it is a banner type. It will have an icon and
/// and a message and will have a background color corresponding to the icon type. 
/// 
/// If the banner is a message, it should appear only as a success banner. It will have an icon and
/// text but will not have a background color.
class UITOOLKIT_PLUGIN_API OriginBanner : public QWidget, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(BannerType)
    Q_ENUMS(IconType)
    Q_PROPERTY(IconType iconType READ getIconType WRITE setIconType)
    Q_PROPERTY(BannerType bannerType READ getBannerType WRITE setBannerType)
    Q_PROPERTY(QString text READ text WRITE setText)


public:
	/// \brief Defines the type of banner
    enum BannerType
    {
          BannerUninitialized ///< The banner type has not been set yet
        , Banner              ///< The banner type is banner and will have a colored background
        , Message             ///< The banner type is message and will not have a color backround
    };

	/// \brief Defines the icon to appear in the banner
    enum IconType
    {
          IconUninitialized ///< The icon has not been set yet
        , Error             ///< This is an error banner
        , Success           ///< This is a success banner
        , Info              ///< This is an informational banner
        , Notice            ///< This is a notice (exclamation point) banner - it does not have a colored background
    };

	/// \brief Constructor
	///
	/// Creates the banner
	/// \param parent The QWidget that is the parent of this object, default is NULL
	/// \param style The icon style that this banner will take, default is Success
	/// \param type The type of banner, default is banner
    OriginBanner(QWidget* parent = NULL, const IconType& style = Success,
                const BannerType& type = Banner);
	/// \brief destructor
    ~OriginBanner();

	/// \brief Gets the icon type
	/// \return IconType The type of icon that has been set
    IconType getIconType() {return mIconType;}

	/// \brief Sets the icon type which sets the style
	/// \param iconType The icon style and type that this banner will take
    void setIconType(IconType iconType);

	/// \brief Gets the banner type
	/// \return BannerType The type of banner
    BannerType getBannerType() const {return mBannerType;}

	/// \brief Sets the banner type
	/// \param bannerType The type of banner
    void setBannerType(BannerType bannerType);


	/// \brief Gets the message text of the banner
	/// \return QString The text contained in the banner
	QString text() const;

	/// \brief Sets the message text of the banner
	/// \param text The text contained in the banner
    void setText(const QString& text);

signals:
    /// \brief Emits a signal if there is a link in the banner and it is clicked
    /// \param const The anchor associated with the link
    void linkActivated(const QString&);

protected:
    QString& getElementStyleSheet();
    void prepForStyleReload();
    bool event(QEvent* event);
    void paintEvent(QPaintEvent* event);
    QWidget* getSelf() {return this;}

	/// \brief Formats the content
	/// 
	/// Formats the icon and message text so that it appears properly
    void formatLabel();


private slots:
	Q_INVOKABLE void adjustWidgetSize (QWidget* );


private:
    static QString mPrivateStyleSheet;
    Ui_OriginBanner* ui;
    IconType mIconType;
    BannerType mBannerType;
};
    }
}

#endif
