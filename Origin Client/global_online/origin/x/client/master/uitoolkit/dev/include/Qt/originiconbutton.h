#ifndef ORIGINICONBUTTON_H_
#define ORIGINICONBUTTON_H_

#include <QtWidgets/QPushButton>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {
/// \brief Provides a square push button control that contains a single, settable icon. The widget has a fixed size of approximately 20x20 pixels.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginIconButton">Click here for more OriginIconButton documentation</A>

class UITOOLKIT_PLUGIN_API OriginIconButton : public QPushButton, public OriginUIBase
{
    Q_OBJECT
    Q_PROPERTY(QPixmap icon READ getIcon WRITE setIcon DESIGNABLE true)

public:
	/// \brief Constructor
	/// \param parent Pointer to the QWidget that is the parent of this button
    OriginIconButton(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginIconButton();

	/// \brief Gets the icon that is displayed in the button
	/// \return QPixmap The icon that is displayed. This can be an empty pixmap.
    const QPixmap& getIcon() const { return icon; }
	/// \brief Sets the icon that is displayed in the button.
	/// 
	/// This image should be PNG image 16x16 pixels or smaller with alpha transparency. The colors in the image provided are ignored.
	/// \param pixmap The PNG image to use for the button.
    void setIcon(const QPixmap& pixmap);

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);
     QPushButton* getSelf() {return this;}
     QString&  getElementStyleSheet();

     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();

private:
    void updateState();

    static QString mPrivateStyleSheet;

    bool    mHovered;
    bool    mPressed;

    QPixmap icon;

    QPixmap* mMask;
};
    }
}
#endif
