#ifndef ORIGINMULTILINEEDIT_H
#define ORIGINMULTILINEEDIT_H

#include <QtWidgets/QFrame>
#include "originmultilineeditcontrol.h"
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Origin {
    namespace UIToolkit {

/// \brief A control used to edit and display both plain and rich text in a multi-line wordwrapped manner.
/// 
/// <A HREF="https://developer.origin.com/documentation/display/EBI/Origin+UI+Toolkit#OriginUIToolkit-OriginMultiLineEdit">Click here for more OriginMultiLineEdit documentation</A>

class UITOOLKIT_PLUGIN_API OriginMultiLineEdit: public QFrame, public OriginUIBase
{
    Q_OBJECT
    Q_PROPERTY(bool background READ isBackgroundEnabled WRITE setBackgroundEnabled)
    Q_PROPERTY(QString html READ getHtml WRITE setHtml)
    Q_PROPERTY(bool autoExpand READ isAutoExpanding WRITE setAutoExpanding)
    Q_PROPERTY(int maxLines READ getMaxLines WRITE setMaxLines)
    //Q_PROPERTY(QString lang READ getCurrentLang WRITE changeLang)

public:

	/// \brief Constructor
	/// \param parent Pointer to the parent widget, defaults to NULL
	OriginMultiLineEdit(QWidget *parent = 0);
	/// \brief Destructor
	~OriginMultiLineEdit() { this->removeElementFromChildList(this);}

	/// \brief Gets whether the background is enabled
	/// 
	/// If set to true, the widget will contain a border indicating focus and validation states. This behavior mimics the OriginSingleLineEdit control. If set to false, the widget will have no border (useful for embedding a section of editable text within another piece of UI)
    bool isBackgroundEnabled() const;
	/// \brief Sets whether the background is enabled
	/// 
	/// If set to true, the widget will contain a border indicating focus and validation states. This behavior mimics the OriginSingleLineEdit control. If set to false, the widget will have no border (useful for embedding a section of editable text within another piece of UI)
	/// \param enabled True to set it enabled, False to disable it	
    void setBackgroundEnabled(bool enabled);

	/// \brief Gets the HTML content
	/// 
	/// This is analogous to the "text" property of OriginSingleLineEdit. The string value supports a subset of the HTML language to enable Rich Text editing within the control, but the html property can also be set to a simple string value.
	/// \return QString The QString containing the HTML.
    QString getHtml() const;
	/// \brief Sets the HTML content
	/// 
	/// This is analogous to the "text" property of OriginSingleLineEdit. The string value supports a subset of the HTML language to enable Rich Text editing within the control, but the html property can also be set to a simple string value.
	/// \param htmlText The QString that contains the HTML text to set.
    void setHtml(QString htmlText);

    /// \brief Gets the plain text context
    /// 
    /// This forces the text to put into plain text and not html
    /// \return QString The QString containing the plain text
    QString getPlainText() const;

	/// \brief Gets the actual widget that displays the text
	/// 
	/// \return OriginMultiLineEditControl A pointer to the text widget inside
    OriginMultiLineEditControl * getMultiLineWidget();

	/// \brief returns the number of lines being displayed
	/// \return int The number of lines
    int getTotalLineCount( );

	/// \brief Gets whether the widget will automatically resize itself.
	/// 
	/// If set to true, the widget will automatically resize itself vertically to only the height required to completely contain the text within the widget. As the text changes, the height will adjust dynamically as appropriate. If set to false, the widget will maintain a fixed height.
	/// NOTE: the autoExpand property should be used in conjunction with the maxLines property in order to prevent the widget from expanding infinitely
	/// \return bool True if the widget should resize, False if it should not.
    bool isAutoExpanding() const;
	/// \brief Sets whether the widget will automatically resize itself.
	/// 
	/// If set to true, the widget will automatically resize itself vertically to only the height required to completely contain the text within the widget. As the text changes, the height will adjust dynamically as appropriate. If set to false, the widget will maintain a fixed height.
	/// NOTE: the autoExpand property should be used in conjunction with the maxLines property in order to prevent the widget from expanding infinitely
	/// \param expanding True if the widget should resize, False if it should not.
    void setAutoExpanding(bool expanding);

	/// \brief Gets the max number of lines allowed in this widget
	/// 
	/// Used in conjunction with the autoExpand property. If the autoExpand property is set to true, maxLines specifies the maximum number of lines that the text can expand. When the maxLines limit has be reached, additional text will create scrollbars within the widget.
	/// This property has no effect if autoExpand is false. It is set to 10 by default.
	/// \return int The number of max lines. 
    int getMaxLines() const;
	/// \brief Sets the max number of lines allowed in this widget
	/// 
	/// Used in conjunction with the autoExpand property. If the autoExpand property is set to true, maxLines specifies the maximum number of lines that the text can expand. When the maxLines limit has be reached, additional text will create scrollbars within the widget.
	/// This property has no effect if autoExpand is false. It is set to 10 by default.
	/// \param lines The number of max lines. 
    void setMaxLines(int lines);
protected:
     virtual void paintEvent(QPaintEvent* event);
     virtual bool event(QEvent* event);

     virtual void keyPressEvent(QKeyEvent* event);
     virtual void resizeEvent(QResizeEvent *e);

     QFrame* getSelf() {return this;}
     QString&  getElementStyleSheet();

     void changeLang(QString newLang);
     QString& getCurrentLang();
     void prepForStyleReload();


protected slots:
     void onChildFocusChanged(bool focusIn);

public slots:
     void adjustWidgetSize(bool resizeEvent = false);
private:
     static QString mPrivateStyleSheet;

     bool m_backgroundEnabled;
     int prevLineCount;
     bool m_autoExpanding;
     int m_maxLines;

     OriginMultiLineEditControl* m_text;

};
    }
}
#endif
