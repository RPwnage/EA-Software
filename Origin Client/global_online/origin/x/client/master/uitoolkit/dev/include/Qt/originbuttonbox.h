#ifndef ORIGINBUTTONBOX_H_
#define ORIGINBUTTONBOX_H_

#include <QDialogButtonBox>
#include <QMap>

#include "uitoolkitpluginapi.h"

namespace Origin {
namespace UIToolkit {
class OriginPushButton;

/// \brief The button box that appears at the bottom of an origin window
class UITOOLKIT_PLUGIN_API OriginButtonBox : public QDialogButtonBox
{
    Q_OBJECT

public:
	/// \brief Constructor
	/// \param parent The parent of the Button Box
    OriginButtonBox(QWidget* parent = 0);

	/// \brief Adds a button to the button box
	/// \param which The type of QDialogButtonBox::StandardButton you wish to add
	/// \return OriginPushButton* A pointer to the newly created button
	OriginPushButton* addButton(const QDialogButtonBox::StandardButton& which);

    // Ways of getting a button or it's info

	/// \brief Get the button associated with the type
	/// \param which The type of button you are looking for
	/// \return OriginPushButton* Pointer to the button
    OriginPushButton* button(const QDialogButtonBox::StandardButton& which) const;
	/// \brief Get the type for the passed in button
	/// \param button Pointer to the button whose type you want to know
	/// \return QDialogButtonBox::StandardButton The type of button
    QDialogButtonBox::StandardButton standardButton(OriginPushButton* button) const;
	/// \brief Get the role for the passed in button.
	/// \return QDialogButtonBox::ButtonRole The role associated with this button
    QDialogButtonBox::ButtonRole buttonRole(OriginPushButton*) const;

    // - Default
	
	/// \brief Sets the default button of the button box
	/// \param which Which type of button to set as the default button
    void setDefaultButton(const QDialogButtonBox::StandardButton& which);
	/// \brief Sets the default button of the button box
	/// \param button Pointer to the button to be set to default
    void setDefaultButton(OriginPushButton* button = NULL);
	/// \brief Gets the default button of the button box
	/// \return OriginPushButton Pointer to the default button
    OriginPushButton* defaultButton() const;

    // - Modify Msg UI
	
	/// \brief Sets the text on the specified button
	/// 
	/// This is a helper method that finds the button specified by the type and sets the text
	/// on that button
	/// \param which Which button to set the text on
	/// \param text The new text of the button
    void setButtonText(const QDialogButtonBox::StandardButton& which, const QString &text);
	/// \brief Toggles the enabled state of all the buttons in the button box
	/// \param enable True if all buttons are to be enabled, False if all buttons are to be disabled
    void setEnableAllButtons(const bool& enable);
	/// \brief Returns a pointer to the last clicked button 
	/// \return OriginPushButton* Returns a pointer to the last clicked button, if no button has been been clicked, returns NULL
    OriginPushButton* getClickedButton() {return mClickedButton;}
	/// \brief Resets the clicked button property
	/// 
    /// This is used in the case where the dialog is reused. It will clear the last clicked state
	// and set it to NULL
    void clearButtonBoxClicked();

signals:
	/// \brief Emitted when a button has been clicked
	/// \param button A pointer to the clicked button
    void buttonClicked(OriginPushButton* button);
	/// \brief Emitted when a button has been added to the button box 
	void buttonAdded();
	

protected slots:
	/// \brief Internal slot used for when a button has been clicked; returns a pointer to the clicked button.
	void buttonBoxClicked(QAbstractButton*);


private:
	QMap<QDialogButtonBox::StandardButton, OriginPushButton*> mButtonMap;
    OriginPushButton* mClickedButton;
};
}
}

#endif