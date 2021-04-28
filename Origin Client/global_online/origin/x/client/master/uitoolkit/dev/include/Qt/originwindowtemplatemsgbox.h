#ifndef ORIGINWINDOWTEMPLATEMSGBOX_H
#define ORIGINWINDOWTEMPLATEMSGBOX_H

#include "originwindowtemplatebase.h"
#include "uitoolkitpluginapi.h"
#include <QDialogButtonBox>

class Ui_OriginWindowContent;

namespace Origin {
namespace UIToolkit {

class OriginMsgBox;
class OriginButtonBox;

/// \brief Provides a user interface for Origin dialog message box widget.
/// Used in OriginWindow. Shouldn't be referenced directly.
class UITOOLKIT_PLUGIN_API OriginWindowTemplateMsgBox : public OriginWindowTemplateBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the QWidget that is the parent of this template
    OriginWindowTemplateMsgBox(QWidget* parent = 0);
    /// \brief Destructor
    ~OriginWindowTemplateMsgBox();

    /// \brief Sets the content of the template.
    /// \param content - The widget content to be displayed in the template.
	void setContent(QWidget* content);
    /// \brief Gets the content widget. Returns NULL if the message box doesn't have content.
    QWidget* getContent() const;

    /// \brief Sets the content of the bottom left area of the template.
    /// \param content - The widget content to be displayed in the bottom left area.
	void setCornerContent(QWidget* content);
    /// \brief Gets the bottom left content widget. 
    /// Returns NULL if the message box doesn't have corner content.
    QWidget* getCornerContent() const;

    /// \brief Adds buttons to the button box. The button box sets up the buttons.
    /// \param buttons - The type of buttons to be added to the button box.
    void addButtons(QDialogButtonBox::StandardButtons buttons);
    /// \brief Returns button box object.
    OriginButtonBox* getButtonBox() const;

    /// \brief Returns message box object.
    OriginMsgBox* getMsgBox() const {return mMsgBox;}
	

protected slots:
    void onButtonAdded();


private:
	OriginMsgBox* mMsgBox;
	Ui_OriginWindowContent* uiWindowContent;
};
}
}
#endif