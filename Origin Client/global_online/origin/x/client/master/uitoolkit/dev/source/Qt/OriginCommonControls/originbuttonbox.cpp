#include "originbuttonbox.h"
#include "originpushbutton.h"
#include <QVariant>
#include <QLayout>

namespace Origin {
namespace UIToolkit {

OriginButtonBox::OriginButtonBox(QWidget* parent)
: QDialogButtonBox(parent)
, mClickedButton(NULL)
{
    // Button spacing should be 10px
#if defined(Q_OS_MAC)
    if(layout())
        layout()->setSpacing(15);
#else
    if(layout())
        layout()->setSpacing(3);
#endif
}


OriginPushButton* OriginButtonBox::addButton(const QDialogButtonBox::StandardButton& which)
{
    QString title ="";
    QDialogButtonBox::ButtonRole role = QDialogButtonBox::InvalidRole;

    // Translate the QDialogButtonBox enum into "title" & "role" values
    //
    // TODO - would be nice to move this lookup into a utility function
    // as it is used in various places in the client (like OriginApp)
    switch(which)
    {
    case QDialogButtonBox::Ok:
        title = tr("ebisu_client_ok_uppercase");
        role = QDialogButtonBox::AcceptRole;
        break;
    case QDialogButtonBox::Retry:
        title = tr("ebisu_client_retry");
        role = QDialogButtonBox::ResetRole;
        break;
    case QDialogButtonBox::Close:
        title = tr("ebisu_client_close");
        role = QDialogButtonBox::RejectRole;
        break;
    case QDialogButtonBox::Cancel:
        title = tr("ebisu_client_cancel");
        role = QDialogButtonBox::RejectRole;
        break;
    case QDialogButtonBox::Yes:
        title = tr("ebisu_client_yes");
        role = QDialogButtonBox::AcceptRole;
        break;
    case QDialogButtonBox::No:
        title = tr("ebisu_client_no");
        role = QDialogButtonBox::RejectRole;
        break;
    default:
        break;
    }

    OriginPushButton* button = new OriginPushButton();
    button->setButtonType((role == QDialogButtonBox::AcceptRole) ? OriginPushButton::ButtonPrimary : OriginPushButton::ButtonSecondary);
    button->setText(title);
    QDialogButtonBox::addButton(button, role);
    mButtonMap.insert(which, button);
    emit(buttonAdded());
    setDefaultButton();
    show();
    return button;
}


OriginPushButton* OriginButtonBox::button(const QDialogButtonBox::StandardButton& which) const
{
    return mButtonMap.value(which);
}


void OriginButtonBox::setButtonText(const QDialogButtonBox::StandardButton& which, const QString &text) 
{
    if (mButtonMap.contains(which))
    {
        mButtonMap.value(which)->setText(text);
    }
}


OriginPushButton* OriginButtonBox::defaultButton() const
{
    // find the current default button, return false if not found
    OriginPushButton *pPushButton = NULL;
    QList<QAbstractButton*> buttonList = QDialogButtonBox::buttons();
    for(QList<QAbstractButton*>::iterator iter(buttonList.begin()); iter!=buttonList.end(); ++iter)
    {
        pPushButton = dynamic_cast<OriginPushButton*>(*iter);

        if (pPushButton && pPushButton->isDefault())
        {
            break;
        }
    }
    return pPushButton;
}


void OriginButtonBox::setDefaultButton(const QDialogButtonBox::StandardButton& which)
{
    // Clear the current default button
    OriginPushButton* pPushButton = defaultButton();
    if (pPushButton)
    {
        pPushButton->setDefault(false);
        pPushButton->setProperty("default", false);
        if (pPushButton->getButtonType() == OriginPushButton::ButtonPrimary)
            pPushButton->setButtonType(OriginPushButton::ButtonSecondary);
    }

    // Set new default button
    if(mButtonMap.contains(which))
    {
        pPushButton = button(which);
        pPushButton->setFocus(Qt::OtherFocusReason);
        pPushButton->setDefault(true);
        if (pPushButton->getButtonType() == OriginPushButton::ButtonSecondary)
            pPushButton->setButtonType(OriginPushButton::ButtonPrimary);
        pPushButton->setProperty("default", true);
    }
}


void OriginButtonBox::setDefaultButton(OriginPushButton* button /*= NULL*/)
{
    // Clear the current default button
    OriginPushButton* pPushButton = defaultButton();
    if (pPushButton)
    {
        pPushButton->setDefault(false);
        pPushButton->setProperty("default", false);
        if (pPushButton->getButtonType() == OriginPushButton::ButtonPrimary)
            pPushButton->setButtonType(OriginPushButton::ButtonSecondary);
    }


    if (button == NULL)
    {
        // Set the first "Ok" button to default
        if(mButtonMap.contains(QDialogButtonBox::Ok))
        {
            OriginPushButton *pPushButton = dynamic_cast<OriginPushButton*>(this->button(QDialogButtonBox::Ok));
            pPushButton->setFocus(Qt::OtherFocusReason);
            pPushButton->setDefault(true);
            if (pPushButton->getButtonType() == OriginPushButton::ButtonSecondary)
                pPushButton->setButtonType(OriginPushButton::ButtonPrimary);
            pPushButton->setProperty("default", true);
        }
        
        // Try the first "Yes" role button to default
        else if(mButtonMap.contains(QDialogButtonBox::Yes))
        {
            OriginPushButton *pPushButton = dynamic_cast<OriginPushButton*>(this->button(QDialogButtonBox::Yes));
            pPushButton->setFocus(Qt::OtherFocusReason);
            pPushButton->setDefault(true);
            if (pPushButton->getButtonType() == OriginPushButton::ButtonSecondary)
                pPushButton->setButtonType(OriginPushButton::ButtonPrimary);
            pPushButton->setProperty("default", true);
        }

        // Just set default to the first button we find
        else
        {
            if(mButtonMap.isEmpty() == false)
            {
                OriginPushButton* pPushButton = dynamic_cast<OriginPushButton*>(*mButtonMap.begin());
                pPushButton->setFocus(Qt::OtherFocusReason);
                pPushButton->setDefault(true);
                if (pPushButton->getButtonType() == OriginPushButton::ButtonSecondary)
                    pPushButton->setButtonType(OriginPushButton::ButtonPrimary);
                pPushButton->setProperty("default", true);
            }
        }
    }
    else
    {
        QList<QAbstractButton*> buttonList = QDialogButtonBox::buttons();
        for(QList<QAbstractButton*>::iterator iter = buttonList.begin(); iter!=buttonList.end(); ++iter)
        {
            if ((*iter) == button)
            {
                OriginPushButton *pPushButton = dynamic_cast<OriginPushButton*>(*iter);
                if (pPushButton)
                {
                    pPushButton->setFocus(Qt::OtherFocusReason);
                    pPushButton->setDefault(true);
                    if (pPushButton->getButtonType() == OriginPushButton::ButtonSecondary)
                        pPushButton->setButtonType(OriginPushButton::ButtonPrimary);
                    pPushButton->setProperty("default", true);
                    break;
                }
            }
        }
    }
}


QDialogButtonBox::StandardButton OriginButtonBox::standardButton(OriginPushButton* button) const
{
    return QDialogButtonBox::standardButton(button);
}


QDialogButtonBox::ButtonRole OriginButtonBox::buttonRole(OriginPushButton* button) const
{
    return QDialogButtonBox::buttonRole(button);
}


void OriginButtonBox::setEnableAllButtons(const bool& enable)
{
    QList<QAbstractButton*> buttonList = QDialogButtonBox::buttons();
    for(QList<QAbstractButton*>::iterator iter = buttonList.begin(); iter != buttonList.end(); ++iter)
        (*iter)->setEnabled(enable);
}


void OriginButtonBox::buttonBoxClicked(QAbstractButton* btn)
{
    mClickedButton = dynamic_cast<OriginPushButton*>(btn);
    emit buttonClicked(mClickedButton); 
}

void OriginButtonBox::clearButtonBoxClicked()
{
    mClickedButton = NULL;
}
}
}
