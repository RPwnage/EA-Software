#ifndef ORIGINRADIOBUTTON_H
#define ORIGINRADIOBUTTON_H

#include <QtWidgets/QRadioButton>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {

/// \brief Radio Button implementation with validation status
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginRadioButton">Click here for more OriginRadioButton documentation</A>

class UITOOLKIT_PLUGIN_API OriginRadioButton : public QRadioButton, public OriginUIBase
{
    Q_OBJECT
   // Q_PROPERTY(QString lang READ getCurrentLang WRITE changeLang)
   Q_ENUMS(OriginValidationStatus)
   Q_PROPERTY(OriginValidationStatus validationStatus READ getValidationStatus WRITE setValidationStatus DESIGNABLE true)

public:
	/// \brief Constructor
	/// \param parent Pointer to the parent widget, defaults to NULL
    OriginRadioButton(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginRadioButton() { this->removeElementFromChildList(this);}

    //
    // Enum that controls what the validation setting should be. This cannot be moved to a lower file
    // because then the Q_ENUM and Q_PROPERTY do not work so instead I have included it here

#include "originvalidationstatus.h"

	/// \brief Sets the validation status for this button.
	/// \param status The validation state of this button
	/// <ul>
	/// <li>Normal indicates that the widget will appear normally
	/// <li>Valid indicates that the widget will be given a green border
	/// <li>Invalid indicates that the widget will be given a red border
	/// <li>Validating is not supported for this widget
	/// </ul>
    void setValidationStatus(const OriginValidationStatus& status);
	/// \brief Returns the current status value
	/// \return OriginValidationStatus The status of the radio button
    OriginValidationStatus getValidationStatus() const;

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);

     QRadioButton* getSelf() {return this;}
     QString&  getElementStyleSheet();


     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();

private:
     static QString mPrivateStyleSheet;
     OriginValidationStatus mStatus;
};

    }
}
#endif
