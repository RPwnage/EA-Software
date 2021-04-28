#ifndef ORIGINLABEL_H
#define ORIGINLABEL_H

#include <QtWidgets/QLabel>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

#include <QMessageBox>

namespace Origin {
    namespace UIToolkit {
/// \brief Provides a simple multi-use label control.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginLabel">Click here for more OriginLabel documentation</A>
class UITOOLKIT_PLUGIN_API OriginLabel : public QLabel, public OriginUIBase
{
    Q_OBJECT
    Q_ENUMS(LabelType)
    Q_ENUMS(HyperlinkStyle)
    Q_ENUMS(SpecialColorStyle)
    Q_ENUMS(OriginValidationStatus)
    Q_ENUMS(Qt::TextElideMode)
    Q_PROPERTY(LabelType labelType READ getLabelType WRITE setLabelType DESIGNABLE true )
    Q_PROPERTY(OriginValidationStatus validationStatus READ getValidationStatus WRITE setValidationStatus DESIGNABLE true)
    Q_PROPERTY(bool hyperlink READ getHyperlink WRITE setHyperlink DESIGNABLE true )
    Q_PROPERTY(HyperlinkStyle hyperlinkStyle READ getHyperlinkStyle WRITE setHyperlinkStyle DESIGNABLE true )
    Q_PROPERTY(SpecialColorStyle specialColorStyle READ getSpecialColorStyle WRITE setSpecialColorStyle DESIGNABLE true )
    Q_PROPERTY(bool hyperlinkArrow READ getHyperlinkArrow WRITE setHyperlinkArrow DESIGNABLE true)
    Q_PROPERTY(Qt::TextElideMode elideMode READ getElideMode WRITE setElideMode DESIGNABLE true)
    Q_PROPERTY(int maxNumLines READ getMaxNumLines WRITE setMaxNumLines DESIGNABLE true)

public:
	/// \brief Constructor
	/// \param parent Pointer to the QWidget that is the parent of this label
	OriginLabel(QWidget *parent = 0);
	/// \brief Destructor
    ~OriginLabel();

	/// \brief Enum that controls what kind of label this is
    enum LabelType { 
		DialogUninitialized, ///< Not yet initialized
		DialogLabel,	     ///< Simple label
		DialogDescription,   ///< Typically describes the purpose of a dialog
		DialogTitle,         ///< Uses a large, heavy font typically for the title of a dialog
		DialogSmall,		 ///< Uses a small font typically for a subscript or de-emphasized text
		DialogStatus,		 ///< Can be set to a normal, valid, or invalid state when it is a status label
		DialogFieldName		 ///< Similar to the DialogLabel but slightly larger
	};

    /// \brief Enum that specifies what the current style of the hyperlink.
    /// It's not useful if the label is not a hyperlink.
    /// TODO: Just switch this to use SpecialColorStyle
    enum HyperlinkStyle
    {
        Uninitialized, ///< Not yet initialized
        Blue,          ///< Set by default.
        Black,         ///< Name and description: TBD
        LightBlue      ///< Set if the background it's on is dark
    };

	/// \brief Enum that specifies what the current state of the hyperlink is
    enum HyperlinkState
    {
        Plain,      ///< Set by default.
        Hover,      ///< Set if cursor is over it.
        Active      ///< Set if pressed.
    };

    /// \brief Enum that specifies a special color style.
    enum SpecialColorStyle
    {
        Color_Normal,           ///< Set by default.
        Color_Red,              ///< OriginValidationStatus: Invalid
        Color_Green,            ///< OriginValidationStatus: Valid
        Color_LightGrey,        ///< On Dark: normally description text
        Color_White,            ///< On Dark: normally title text
        Color_Blue,             ///< Hyperlink: Normal case
        Color_Black,            ///< Hyperlink: Special case - don't use this normally
        Color_LightBlue         ///< Hyperlink: Set if the background it's on is dark
    };

    // 
    // Enum that controls what the validation setting should be. This cannot be moved to a lower file
    // because then the Q_ENUM and Q_PROPERTY do not work so instead I have included it here

#include "originvalidationstatus.h"

	/// \brief Sets the type of a label
	/// \param labelType The type of label to set this dialog to.
    void setLabelType(LabelType labelType);
	/// \brief Gets the label type
	/// \return labelType The type of label this has been set to
    LabelType getLabelType() const { return mLabelType; }

	/// \brief Sets whether this is a hyperlink
	/// 
	/// When set to "true", gives the label hyperlink functionality: a pointing "hand" cursor when moused over, a differentiating color to the text, and an underline when the label is hovered. 
	/// The label appears clickable to the user and interacts with the mouse.
	/// \param aHyperlink True if a hyperlink, False if not
    void setHyperlink(bool aHyperlink);
	/// \brief Gets whether this is a hyperlink
	/// \return bool True if a hyperlink, False if not
    bool getHyperlink() const { return mHyperlink; }

    /// \brief Sets the hyperlink style. This is only effective if the label is
    /// a hyperlink (see:setHyperlink)
    /// 
    /// Blue is the default
    /// \param aHyperlinkStyle The type of style the hyperlink will be set to
    void setHyperlinkStyle(HyperlinkStyle aHyperlinkStyle);
    /// \brief Gets the style of the hyperlink
    HyperlinkStyle getHyperlinkStyle() const { return mHyperlinkStyle; }

    /// \brief Sets a special color style.
    /// 
    /// Blue is the default
    /// \param aHyperlinkStyle The type of style the hyperlink will be set to
    void setSpecialColorStyle(SpecialColorStyle aColorStyle);
    /// \brief Gets the style of the hyperlink
    SpecialColorStyle getSpecialColorStyle() const { return mSpecialColorStyle; }

	/// \brief Sets whether an arrow should be used with the hyperlink
	/// 
	/// When set to "true" and the label is also configured as a hyperlink, it places a small arrow graphic to the right of the label text.
	/// \param aHyperlinkArrow True if there should be an arrow, False if not
	void setHyperlinkArrow(bool aHyperlinkArrow);
	/// \brief Gets whether this is a hyperlink arrow
	/// \return bool True if there is a hyperlink arrow, False if not
    bool getHyperlinkArrow() const { return mHyperLinkArrowWidget != NULL; }

	/// \brief Sets where Ellipsis should appear when displaying text larger than the area allowed. NOTE: if maxLines is
    /// 
	/// <A HREF="http://doc.qt.nokia.com/4.8-snapshot/qt.html#TextElideMode-enum">Click here for Qt documentation</A>
	/// \param aElideMode Qt enum for the setting
	/// <ul>
	/// <li>Qt::ElideLeft for ellipse at the beginning of the text
	/// <li>Qt::ElideRight for ellipse at the end of the text
	/// <li>Qt::ElideMiddle for ellipse in the middle fo the text
	/// <li>Qt::ElideNone for no ellipse appearing in the text
	/// </ul>
	void setElideMode(Qt::TextElideMode aElideMode);
	/// \brief Gets the setting for where the Ellipsis should appear
	/// 
	/// <A HREF="http://doc.qt.nokia.com/4.8-snapshot/qt.html#TextElideMode-enum">Click here for Qt documentation</A>
	/// \returns Qt::TextElideMode Enum Qt enum for the setting
	/// <ul>
	/// <li>Qt::ElideLeft for ellipse at the beginning of the text
	/// <li>Qt::ElideRight for ellipse at the end of the text
	/// <li>Qt::ElideMiddle for ellipse in the middle fo the text
	/// <li>Qt::ElideNone for no ellipse appearing in the text
	/// </ul>
    Qt::TextElideMode getElideMode() const { return mElideMode; }

    /// \brief returns true if the text is Ellipsised
	bool isShowingElide() const;

    /// \brief Sets the validation status for this container. 
	/// \param status The validation status to set
	/// <ul>
	/// <li>Normal - the label color will appear normally
	/// <li>Valid - the label color will change to green
	/// <li>Invalid - the label color will change to red
	/// <li>Validating - not supported for this widget
	/// </ul>
    void setValidationStatus(const OriginValidationStatus& status);
	/// \brief Returns the current validation status for this container. 
	/// \return OriginValidationStatus The current validation status
	/// <ul>
	/// <li>Normal - the label color will appear normally
	/// <li>Valid - the label color will change to green
	/// <li>Invalid - the label color will change to red
	/// </ul>
    OriginValidationStatus getValidationStatus() const;

    /// \brief Sets the maximum number of lines that should be used in this label
    /// \param numLines The maximum number of lines
    void setMaxNumLines(const int& numLines);
    /// \brief Returns the maximum number of lines that should be used in this label
    /// \return int The maximum number of lines
    int getMaxNumLines() const {return mMaxNumLines;}

    void setText(const QString& text);


signals:
	/// \brief Signal sent when a click has been registered on this widget
    void clicked();

protected:
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* event);

    virtual void updateHyperlinkState(HyperlinkState state);

    QLabel* getSelf() {return this;}
    QString&  getElementStyleSheet();

    void changeLang(QString newLang);
    QString& getCurrentLang();

    void prepForStyleReload();

    void setKerning(bool enable);


private:
    static QString mPrivateStyleSheetDialogLabel;
    static QString mPrivateStyleSheetDialogDescription;
    static QString mPrivateStyleSheetDialogTitle;
    static QString mPrivateStyleSheetDialogSmall;
    static QString mPrivateStyleSheetDialogFieldName;
    static QString mPrivateStyleSheetDialogStatus;

    QString mCurrentStyleSheet;

    LabelType mLabelType;
    OriginValidationStatus mStatus;
    Qt::TextElideMode mElideMode;
    HyperlinkStyle mHyperlinkStyle;
    SpecialColorStyle mSpecialColorStyle;
    bool mHyperlink;
    int mMaxNumLines;

    bool    mHovered;
    bool    mPressed;
    bool    mFocusRect;
    QLabel* mHyperLinkArrowWidget;
    static const int ARROW_OFFSET_X = 7;
    int ARROW_OFFSET_Y;
    int mIsShowingElide;
};

}
}
#endif
