#ifndef ORIGINCOMMANDLINK_H
#define ORIGINCOMMANDLINK_H

#include <QtWidgets/QPushButton>
#include "originuiBase.h"
#include "originlabel.h"
#include "uitoolkitpluginapi.h"

namespace Ui{
class OriginCommandLink;
}

namespace Origin {
    namespace UIToolkit {
/// \brief Widget that is derived from a QPushButton. It contains text, description and a set icon. 
///
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginCommandLink">Click here for more OriginCommandLink documentation</A>
class UITOOLKIT_PLUGIN_API OriginCommandLink : public QPushButton, public OriginUIBase
{
    Q_OBJECT
    Q_PROPERTY(QString text READ getText WRITE setText)
    Q_PROPERTY(QString description READ getDescription WRITE setDescription)
    Q_PROPERTY(bool fixedHeight READ getFixedHeightProp WRITE setFixedHeightProp)

public:
	/// \brief Constructor
	/// \param parent Pointer to the QWidget that is the parent of this object
    explicit OriginCommandLink(QWidget *parent=0);
	/// \brief Constructor
	/// \param text QString that is the text that should appear at the top of this button area
	/// \param parent Pointer to the QWidget that is the parent of this object
    explicit OriginCommandLink(const QString &text, QWidget *parent=0);
	/// \brief Constructor
	/// \param text QString that is the text that should appear at the top of this button area
	/// \param description QString that is the description that appears under the text 
	/// \param parent Pointer to the QWidget that is the parent of this object
    OriginCommandLink(const QString &text, const QString &description, QWidget *parent=0);
	/// \brief Destructor
    ~OriginCommandLink();

	/// \brief Gets the text value
	/// \return QString The contents of the text field
    QString getText() const { return text; }
	/// \brief Sets the text value
	/// \param aText The QString that should appear in the text field 
    void setText(const QString &aText);

	/// \brief Gets the description value
	/// \return QString The contents of the desciption field
    QString getDescription() const { return description; }
    void setDescription(const QString &aDescription);

	/// \brief Gets the fixed height property
	/// \return bool True if it has been set, false if it has not
    bool getFixedHeightProp() const { return fixedHeight; }
	/// \brief Sets the fixed height property
	/// 
	/// If true, the control will assign itself a fixed height equal to exactly the height
    /// needed by the contents (the text and description fields). If false, the control will
    /// have a flexible height with a minimum height of the height of the contents.
	/// \param bool True to set it, false to not set a fixed height.
    void setFixedHeightProp(bool);

protected:
    virtual QSize minimumSizeHint () const { return QSize(width(), getWidgetHeight()); }
    virtual QSize sizeHint() const;
    bool event(QEvent *e);
    void paintEvent(QPaintEvent *);

private:

    Ui::OriginCommandLink *ui;

    void init();

    qreal titleSize() const;

    QFont titleFont() const;
    QFont descriptionFont() const;

    QString text;
    QString description;
    bool fixedHeight;

    int getWidgetHeight() const;
    void updateFixedHeight();

    QPushButton* getSelf() {return this;}
    QString&  getElementStyleSheet();

    void changeLang(QString newLang);
    QString& getCurrentLang();
    void prepForStyleReload();

    static QString mPrivateStyleSheet;

};
    }

}


#endif
