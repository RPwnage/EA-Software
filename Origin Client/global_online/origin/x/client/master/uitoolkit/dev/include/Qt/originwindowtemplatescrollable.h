#ifndef ORIGINWINDOWTEMPLATESCROLLABLE_H
#define ORIGINWINDOWTEMPLATESCROLLABLE_H

#include "originwindowtemplatebase.h"
#include "uitoolkitpluginapi.h"
#include <QDialogButtonBox>

class QScrollArea;
class Ui_OriginWindowContent;

namespace Origin {
namespace UIToolkit {

class OriginButtonBox;

/// \brief Provides a user interface for Origin dialog scrollable widget.
/// Used in OriginWindow. Shouldn't be referenced directly.
class UITOOLKIT_PLUGIN_API OriginWindowTemplateScrollable : public OriginWindowTemplateBase
{
    Q_OBJECT

public:
    /// \brief Constructor
    /// \param parent Pointer to the QWidget that is the parent of this template
	OriginWindowTemplateScrollable(QWidget* parent = 0);
    /// \brief Destructor
    ~OriginWindowTemplateScrollable();

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


protected slots:
    void adjustScrollAreaSize(int min, int max);
    void onButtonAdded();


private:
	QScrollArea* mScrollArea;
	Ui_OriginWindowContent* uiWindowContent;
};
}
}
#endif