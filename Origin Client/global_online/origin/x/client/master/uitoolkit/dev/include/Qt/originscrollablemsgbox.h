#ifndef ORIGINSCROLLABLEMSGBOX_H
#define ORIGINSCROLLABLEMSGBOX_H

#include <QtWidgets/QWidget>
#include <QDialogButtonBox>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

class QLabel;

namespace Ui {
    class OriginScrollableMsgBox;
}

namespace Origin {
namespace UIToolkit {

class OriginButtonBox;
class OriginPushButton;
/// \brief Provides a user interface for Origin common message box.
/// After the message box height exceeds 400px - it will begin to scroll.
class UITOOLKIT_PLUGIN_API OriginScrollableMsgBox : public QWidget, public OriginUIBase
{
    Q_OBJECT

public:
    /// \brief Enum that controls what kind of icon image will show
    enum IconType 
    { 
        NoIcon
        , Error
        , Info
        , Notice
        , Success 
    };


    /// \brief Constructor
    /// \param icon - The type of icon image that should be shown.
    /// \param title - The title of the message box.
    /// \param text - The text to be displayed in the message box.
    /// \param parent - Pointer to the QWidget that is the parent of this message box.
    OriginScrollableMsgBox(IconType icon = NoIcon, const QString &title = "", const QString &text = "", QWidget *parent = 0);
    
    /// \brief Destructor
    ~OriginScrollableMsgBox();

    /// \brief Sets the type of a icon.
    /// \param icon - The type of icon image that should be shown.
    /// \param title - The title of the message box.
    /// \param text - The text to be displayed in the message box.
    void setup(IconType icon, const QString& title, const QString& text);

    // Ui
    /// \brief Sets the type of a icon.
    /// \param icon The type of icon image that should be shown
    void setIcon(IconType icon);

    /// \brief Sets the title of the message box
    /// \param title The string the title label will be set to.
    void setTitle(const QString& title);

    /// \brief Returns the title of the message box
    const QString getTitle() const;

    /// \brief Returns the QLabel of the message box�s description text.
    /// Used if the user wants the customize the QLabels properties.
    /// e.g. Making text selectable or able to open external links.
    QLabel* getTextLabel();

    /// \brief Sets the text of the message box
    /// \param text The string the text label will be set to.
    void setText(const QString &text);

    /// \brief Returns the text of the message box
    const QString getText() const;

    // Content
    /// \brief Sets the content of the message box.
    /// \param content - The widget content to be displayed in the message box.
    void setContent(QWidget* content);

    /// \brief Removes and deletes the content widget.
    void removeContent();

    /// \brief Gets the content widget. Returns NULL if the message box doesn't have content.
    QWidget* const content() {return mContent;}

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

    // Ways of getting a button or it's info
    /// \brief (Template - MsgBox & Scrollable) Get the button associated with the type
    /// \param which - The type of button you are looking for
    /// \return OriginPushButton* Pointer to the button
    OriginPushButton* button(QDialogButtonBox::StandardButton which) const;

    // - Modify Msg UI
    /// \brief (Template - MsgBox & Scrollable) Sets the text on the specified button
    /// 
    /// This is a helper method that finds the button specified by the type and sets the text
    /// on that button
    /// \param which Which button to set the text on
    /// \param text The new text of the button
    void setButtonText(QDialogButtonBox::StandardButton which, const QString &text);

    // - Default
    /// \brief (Template - MsgBox & Scrollable) Sets the default button of the button box.
    /// \param which - type of button to set as the default button.
    void setDefaultButton(QDialogButtonBox::StandardButton which);

    /// \brief Sets if the message box should have a dynamic width to it's content.
    void setHasDynamicWidth(const bool& dynamicWidth);


protected:
    void paintEvent(QPaintEvent* event);

    bool event(QEvent* event);
    bool eventFilter(QObject* obj, QEvent* event);
    QWidget* getSelf() {return this;}
    QString&  getElementStyleSheet();
    void prepForStyleReload();
    void adjustForText();
			  
protected slots:
    void adjustScrollAreaHeight(int min, int max);
	void adjustToScrollAreaWidth(int min, int max);
    void onButtonAdded();

private:
    Ui::OriginScrollableMsgBox* ui;
    static QString mPrivateStyleSheet;
    QWidget* mContent;
};
}
}

#endif
