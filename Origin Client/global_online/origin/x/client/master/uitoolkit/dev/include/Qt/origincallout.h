#ifndef ORIGINCALLOUT_H_
#define ORIGINCALLOUT_H_

#include <QDialog>
#include "originuiBase.h"
#include "uitoolkitpluginapi.h"

namespace Ui
{
    class OriginCallout;
}

namespace Origin
{
namespace UIToolkit
{
class OriginCalloutIconTemplateContentLine;

class UITOOLKIT_PLUGIN_API OriginCallout : public QDialog, public OriginUIBase
{
	Q_OBJECT
    Q_ENUMS(CalloutTemplate)
    Q_PROPERTY(CalloutTemplate calloutTemplate READ calloutTemplate WRITE setCalloutTemplate)
    Q_PROPERTY(QString bindWidget READ bindWidgetName WRITE setBindWidget)
public:

    enum CalloutTemplate
    {
        Callout_Plain = 1, 
        Callout_BlueBorder,
        Callout_Grey
    };

    OriginCallout(const CalloutTemplate& calloutTemplate = Callout_Plain);
    OriginCallout(const QString& bindObjName, const CalloutTemplate& calloutTemplate = Callout_Plain);
	OriginCallout(QWidget* bindObj, const CalloutTemplate& calloutTemplate = Callout_Plain);
	~OriginCallout();

    void setBindWidget(const QString& objName);
    void setBindWidget(QWidget* widget);
    QWidget* bindWidget() const {return mBindWidget;}
    const QString bindWidgetName() const {return mBindWidget ? mBindWidget->objectName() : "";}

    void setContent(QWidget* content);
    QWidget* content() {return mContent;}
    /// \brief Removes the content of the callout and deletes it 
    void removeContent();
    /// \brief Removes the content of the callout, but doesn't delete it.
    void takeContent();

    void setCalloutTemplate(const CalloutTemplate& calloutTemplate);
    const CalloutTemplate calloutTemplate() const {return mCalloutTemplate;}

    /// \brief Updating the callout position to the binded widget
    void positionToBinded();

    static const QString formatTitleForBlue(const QString& blueText, const QString& text);
    static OriginCallout* infoTipTemplate(QWidget* bindObj, const QString& text);
    static OriginCallout* infoTipTemplate(QWidget* bindObj, QWidget* content);
    static OriginCallout* calloutIconTemplate(const QString& bindObjName, const QString& blueTitle, const QString& title, QWidget* content);
    static OriginCallout* calloutIconTemplate(const QString& bindObjName, const QString& blueTitle, const QString& title, QList<UIToolkit::OriginCalloutIconTemplateContentLine*> contentLines);
    static OriginCallout* calloutIconTemplate(QWidget* bindObj, const QString& blueTitle, const QString& title, QWidget* content);
    static OriginCallout* calloutIconTemplate(QWidget* bindObj, const QString& blueTitle, const QString& title, QList<UIToolkit::OriginCalloutIconTemplateContentLine*> contentLines);

signals:
    void dyingFromParent();


protected:
	QString& getElementStyleSheet();
    void prepForStyleReload() {mPrivateStyleSheet = "";}
    void paintEvent(QPaintEvent* event);
    bool event(QEvent* e);
    bool eventFilter(QObject* obj, QEvent* e);
    QDialog* getSelf() {return this;}

private:
    void init(const CalloutTemplate& calloutTemplate = Callout_Plain);

	static QString mPrivateStyleSheet;
    QWidget* mBindWidget;
    QWidget* mContent;
    Ui::OriginCallout* ui;
    CalloutTemplate mCalloutTemplate;
};
}
}
#endif