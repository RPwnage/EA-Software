#ifndef ORIGINMULTILINEEDITCONTROL_H
#define ORIGINMULTILINEEDITCONTROL_H

#include <QtWidgets/QTextEdit>
#include <QtWidgets/QFrame>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {

/// \brief A widget that handles the text inside of a OriginMultiLineEdit
class UITOOLKIT_PLUGIN_API OriginMultiLineEditControl : public QTextEdit, public OriginUIBase
{
    Q_OBJECT
    //Q_PROPERTY(QString lang READ getCurrentLang WRITE changeLang)

public:
	/// \brief Constructor
	/// \param parent Pointer to the parent widget, defaults to NULL
    OriginMultiLineEditControl(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginMultiLineEditControl() { this->removeElementFromChildList(this);}

	/// \brief Gets whether or not the background- this is a property set through designer
	/// \return True if set, False if not
    bool isBackgroundEnabled() const;
	/// \brief Sets whether or not the background- this is a property set through designer
	/// \param enabled True if set, False if not
    void setBackgroundEnabled(bool enabled);
	/// \brief Overriden function to handle the keypress event
	/// \param event The event associated with this keypress
    void keyPressEvent(QKeyEvent* event);

signals:
	/// \brief Signal to indicate either this widget gained or lost focus
	/// \param focusIn True if focus was gained, False if it lost focus
    void objectFocusChanged(bool focusIn);

protected:
     void paintEvent(QPaintEvent* event);
     bool event(QEvent* event);

     QTextEdit* getSelf() {return this;}
     QString&  getElementStyleSheet();


     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();

private:
     static QString mPrivateStyleSheet;

};
    }
}
#endif
