#ifndef ORIGINSINGLELINEEDIT_H
#define ORIGINSINGLELINEEDIT_H

#include <QtWidgets/QLineEdit>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {

/// \brief A one-line text editor which allows the user to enter and edit a single line of plain text with a useful collection of editing functions, including undo and redo, cut and paste, and drag and drop.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginSingleLineEdit">Click here for more OriginSingleLineEdit documentation</A>

class UITOOLKIT_PLUGIN_API OriginSingleLineEdit : public QLineEdit, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(OriginValidationStatus)
    Q_ENUMS(OriginInputAccessory)
    Q_PROPERTY(bool password READ passwordView WRITE setPasswordView)
    Q_PROPERTY(OriginValidationStatus validationStatus READ getValidationStatus WRITE setValidationStatus DESIGNABLE true)
    Q_PROPERTY(OriginInputAccessory accessory READ getInputAccessory WRITE setInputAccessory DESIGNABLE true)
   // Q_PROPERTY(QString lang READ getCurrentLang WRITE changeLang)

public:

    //this is defined once but needs to be included in each place. This is the validation status enum.
    // Validating is not an option for this widget.
#include "originvalidationstatus.h"

    /// \brief Defines the type of accessory items to have at the end of the widget. Default will be None
    enum OriginInputAccessory
    {
        None,      ///< No accessory item
        Search,    ///< Search icon
    };

    /// \brief Constructor
    /// \param parent Pointer to the parent widget, defaults to NULL    
    OriginSingleLineEdit(QWidget *parent = 0);
    /// \brief Destructor
    ~OriginSingleLineEdit() { this->removeElementFromChildList(this);}
    /// \brief Gets if this a password view
    /// 
    /// If true, all text typed within the widget will be obscured by "bullet" characters (Typically a "*" symbol, but actual symbol used is locale and implementation specific)
    /// \return True if it is password view, False if not
    bool passwordView() const;
    /// \brief Sets if this a password view
    /// 
    /// If true, all text typed within the widget will be obscured by "bullet" characters (Typically a "*" symbol, but actual symbol used is locale and implementation specific)
    /// \param enabled True if it is password view, False if not
    void setPasswordView(bool enabled);
    
    /// \brief Sets the validation status for this line edit
    /// \param status The validation state of this line edit
    /// <ul>
    /// <li>Normal indicates that the widget will appear normally
    /// <li>Valid indicates that the widget will be given a green border
    /// <li>Invalid indicates that the widget will be given a red border
    /// <li>Validating is not supported for this widget
    /// </ul>
    void setValidationStatus(const OriginValidationStatus& status);
    /// \brief Returns the current status value
    /// \return OriginValidationStatus The status of the line edit
    OriginValidationStatus getValidationStatus() const;

    /// \brief Sets the input accessory
    ///
    /// Specifies whether there is an input accessory at the end of the widget. If an accessory is set, clicking on this accessory will emit a signal. Default is None.
    /// \param inputAccessory OriginInputAccessory to set the input accessory
    void setInputAccessory(const OriginInputAccessory& inputAccessory);
    /// \brief Gets the input accessory
    ///
    /// Get whether there is an input accessory at the end of the widget. If an accessory is set, clicking on this accessory will emit a signal. Default is None.
    /// \return inputAccessory OriginInputAccessory of the current input accessory value.
    OriginInputAccessory getInputAccessory() const;

signals:
    /// \brief Signal emitted if there is a search accessory and it has been clicked on
    void accessoryClicked();

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);
     void mousePressEvent(QMouseEvent* e);
     void mouseReleaseEvent(QMouseEvent* e);
     void mouseMoveEvent(QMouseEvent *e);
     void focusOutEvent(QFocusEvent *e);
     void resizeEvent(QResizeEvent* e);

     QLineEdit* getSelf() {return this;}
     QString&  getElementStyleSheet();

     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();

private:
     static QString mPrivateStyleSheet;
     bool m_passwordView;
     /// \brief the current status of the widget
     OriginValidationStatus mStatus;
     /// \brief whether or not there is an accessory at the end
     OriginInputAccessory mAccessory;
     /// \brief a variable to store the pixmap so it is only created once
     ///
     /// Cannot be static because of the order Qt creates things
     QPixmap mSearchPixmap;
     /// \brief variable to store the rectangle so that it only needs to be 
     /// determined once
     QRect   mSearchRect; 
     /// \brief whether the mouse received a press event
     bool mMouseDown;
     /// \brief initialized at creation time with the input text cursor
     /// this will get replaced with an arrow when you hover over the accessory (if it exists)
     const QCursor mOrigCursor;
};
    }
}

#endif
