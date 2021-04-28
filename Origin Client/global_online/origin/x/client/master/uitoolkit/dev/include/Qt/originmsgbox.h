#ifndef ORIGINMSGBOX_H
#define ORIGINMSGBOX_H

#include <QtWidgets/QFrame>
#include <QtWidgets/QWidget>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

class QLabel;

namespace Ui {
    class OriginMessageBox;
}

namespace Origin {
    namespace UIToolkit {
class OriginLabel;
/// \brief Provides a user interface for Origin common message box.
class UITOOLKIT_PLUGIN_API OriginMsgBox : public QFrame, public OriginUIBase
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
    /// \param content - The widget content to be displayed in the message box.
    /// \param parent - Pointer to the QWidget that is the parent of this message box.
    OriginMsgBox(QWidget* content = NULL, QWidget *parent = NULL);

    /// \brief Constructor
    /// \param icon - The type of icon image that should be shown.
    /// \param title - The title of the message box.
    /// \param text - The text to be displayed in the message box.
    /// \param content - The widget content to be displayed in the message box.
    /// \param parent - Pointer to the QWidget that is the parent of this message box.
    OriginMsgBox(IconType icon, const QString &title, const QString &text, 
                QWidget* content = NULL, QWidget *parent = 0);
    
    /// \brief Destructor
    ~OriginMsgBox();

    /// \brief TODO: Remove not needed.
    virtual void retranslate();

    /// \brief Sets the type of a icon.
    /// \param icon - The type of icon image that should be shown.
    /// \param title - The title of the message box.
    /// \param text - The text to be displayed in the message box.
    /// \param content - The widget content to be displayed in the message box.
    void setup(IconType icon, const QString &title, const QString &text, QWidget* content = NULL);


    // Ui
    /// \brief Sets the type of a icon.
    /// \param icon The type of icon image that should be shown
    void setIcon(IconType icon);

    /// \brief Sets the title of the message box
    /// \param title The string the title label will be set to.
    void setTitle(const QString& title);

    /// \brief Returns the title of the message box
    const QString getTitle() const;

    /// \brief Returns the QLabel of the message box’s title.
    /// Used if the user wants the customize the QLabels properties.
    /// e.g. Making text selectable or able to open external links.
    OriginLabel* getTitleLabel();

    /// \brief Returns the QLabel of the message box’s description text.
    /// Used if the user wants the customize the QLabels properties.
    /// e.g. Making text selectable or able to open external links.
    QLabel* getTextLabel();

    /// \brief Sets the text of the message box
    /// \param text The string the text label will be set to.
    void setText(const QString &text);

    /// \brief Returns the text of the message box
    const QString getText() const;

    /// \brief Gets the debug text of the message box
    const QString& userInfo() const {return mUserInfo;}

    /// \brief Sets the debug text of the message box
    /// \param userInfo - The string the debug text will be set to.
    void setUserInfo(const QString &userInfo) {mUserInfo = userInfo;}


    // Content
    /// \brief Sets the content of the message box.
    /// \param content - The widget content to be displayed in the message box.
    void setContent(QWidget* content);

    /// \brief Removes and deletes the content widget.
    void removeContent();

    /// \brief Gets the content widget. Returns NULL if the message box doesn't have content.
    QWidget* const content() {return mContent;}


protected:
    void paintEvent(QPaintEvent* event);
    virtual void changeEvent( QEvent* event );

    bool event(QEvent* event);
    QFrame* getSelf() {return this;}
    QString&  getElementStyleSheet();
    void prepForStyleReload();

    void init(IconType icon = NoIcon, const QString &title = QString(), 
              const QString &text = QString(), QWidget* content = NULL);

    void layoutMsgBox();

    QString mUserInfo;

private:
    Ui::OriginMessageBox *ui;
    static QString mPrivateStyleSheet;
    IconType mIconType;
    QWidget* mContent;
};
    }
}

#endif