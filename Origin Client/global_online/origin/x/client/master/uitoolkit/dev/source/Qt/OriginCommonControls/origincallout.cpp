#include "origincallout.h"
#include "ui_origincallout.h"
#include "originlabel.h"
#include "origincallouticontemplate.h"
#include <QMouseEvent>

namespace Origin
{
namespace UIToolkit
{

QString OriginCallout::mPrivateStyleSheet = QString("");

OriginCallout::OriginCallout(const CalloutTemplate& calloutTemplate)
: QDialog(NULL)
{
    init(calloutTemplate);
}


OriginCallout::OriginCallout(const QString& bindObjName, const CalloutTemplate& calloutTemplate) 
: QDialog(NULL)
{
    init(calloutTemplate);
    setBindWidget(bindObjName);
}


OriginCallout::OriginCallout(QWidget* bindObj, const CalloutTemplate& calloutTemplate) 
: QDialog(NULL)
{
    init(calloutTemplate);
    setBindWidget(bindObj);
}


void OriginCallout::init(const CalloutTemplate& calloutTemplate)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::WindowSystemMenuHint);
    mBindWidget = NULL;
    mContent = NULL;
    ui = new Ui::OriginCallout();
    ui->setupUi(this);
    setOriginElementName("OriginCallout");
    m_children.append(this);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    mCalloutTemplate = calloutTemplate;
    setCalloutTemplate(mCalloutTemplate);
}


OriginCallout::~OriginCallout() 
{
    // Disconnect all connections to the callout
    disconnect();
    removeContent();
	removeElementFromChildList(this);

    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void OriginCallout::setContent(QWidget* content)
{
    if(content)
    {
        removeContent();
        mContent = content;
        ui->contentLayout->addWidget(mContent, 1);
        ui->carrotTop->raise();
        ui->carrotBottom->raise();
        adjustSize();
    }
}


void OriginCallout::removeContent()
{
    if(mContent)
    {
        ui->contentLayout->removeWidget(mContent);
        mContent->deleteLater();
        mContent = NULL;
    }
}


void OriginCallout::takeContent()
{
    if(mContent)
    {
        ui->contentLayout->takeAt(ui->contentLayout->indexOf(mContent));
        mContent->setParent(NULL);
        mContent = NULL;
    }
}


void OriginCallout::setCalloutTemplate(const CalloutTemplate& calloutTemplate)
{
    mCalloutTemplate = calloutTemplate;
    QVariant variant = QVariant::Invalid;
    switch(mCalloutTemplate)
    {
    case Callout_Plain:
        variant = "plain";
        ui->gridLayout->setRowMinimumHeight(0, 9);
        ui->gridLayout->setRowMinimumHeight(4, 9);
        ui->contentLayout->setContentsMargins(1, 1, 1, 1);
        break;

    case Callout_BlueBorder:
        variant = "blue-border";
        ui->gridLayout->setRowMinimumHeight(0, 12);
        ui->gridLayout->setRowMinimumHeight(4, 12);
        ui->contentLayout->setContentsMargins(3, 3, 3, 3);
        break;

    case Callout_Grey:
        variant = "grey";
        ui->gridLayout->setRowMinimumHeight(0, 9);
        ui->gridLayout->setRowMinimumHeight(4, 9);
        ui->contentLayout->setContentsMargins(10, 5, 10, 5);
        break;

    default:
        break;
    }
    setProperty("template", variant);
    setStyleSheet(this->styleSheet());
}


bool OriginCallout::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QDialog::event(event);
}


void OriginCallout::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QDialog::paintEvent(event);
}


bool OriginCallout::eventFilter(QObject* obj, QEvent* e)
{
    const QEvent::Type ty = e->type();
    if(mBindWidget)
    {
        if(obj == mBindWidget)
        {
            switch(ty)
            {
            case QEvent::MouseButtonRelease:
                {
                    QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(e);
                    if(mouseEvent && mouseEvent->button() == Qt::LeftButton)
                    {
                        // Remove the event filters to avoid flicker
                        mBindWidget->topLevelWidget()->removeEventFilter(this);
                        mBindWidget->removeEventFilter(this);
                        accept();
                    }
                }
                break;
            case QEvent::Destroy:
                {
                    emit dyingFromParent();
                }
                break;
            default:
                break;
            }
        }
        else if(mBindWidget->topLevelWidget() && obj == mBindWidget->topLevelWidget())
        {
            switch(ty)
            {
            case QEvent::Resize:
            case QEvent::LayoutRequest:
                {
                    positionToBinded();
                }
                break;
            default:
                break;
            }
        }

    }
    return QDialog::eventFilter(obj, e);
}


void OriginCallout::setBindWidget(const QString& objName)
{
    QWidget* obj = NULL;
    foreach (QWidget* widget, QApplication::topLevelWidgets())
    {
        obj = widget->findChild<QWidget*>(objName);
        if(obj)
            break;
    }
    setBindWidget(obj);
}


void OriginCallout::setBindWidget(QWidget* widget)
{
    mBindWidget = widget;
    if(mBindWidget && mBindWidget->topLevelWidget())
    {
        setParent(mBindWidget->topLevelWidget());
        mBindWidget->topLevelWidget()->installEventFilter(this);
        mBindWidget->installEventFilter(this);
        positionToBinded();
    }
}


void OriginCallout::positionToBinded()
{
    setUpdatesEnabled(false);
    // Update if the background image changes
    const int dropShadowWidth = (9 + 10) / 2;
    const int actualWidth = width() - dropShadowWidth;
    QSizePolicy::Policy rs = QSizePolicy::Expanding;
    QSizePolicy::Policy ls = QSizePolicy::Expanding;
    QVariant directionVariant = QVariant::Invalid;
    QVariant levelVariant = QVariant::Invalid;

    // mapped top,left from window + (half width, height)
    const QPoint centerOfBinded = mBindWidget->mapTo(mBindWidget->topLevelWidget(), QPoint(0,0)) + QPoint((mBindWidget->width()/2), mBindWidget->height());
    
    // Map it to the web objects bottom center
    QPoint p = centerOfBinded - QPoint(actualWidth/2, 0);
    
    // See if the callout would be cut off if we were to position it in the center
    const int offscreenRight = p.x() + actualWidth;
    if(mBindWidget->topLevelWidget()->width() < offscreenRight)
    {
        p = centerOfBinded - QPoint(actualWidth, 0);
        directionVariant = "right";
        rs = QSizePolicy::Fixed;
        // If we are still offscreen, just push over.. We really are kind of in the middle because the callout is too big.
        if(p.x() < 0)
        {
            const int leftOffset = mBindWidget->topLevelWidget()->width() - width();
            p.setX(leftOffset/2);
            directionVariant = QVariant::Invalid;
        }
    }
    else if(p.x() < 0)
    {
        p = centerOfBinded;
        directionVariant = "left";
        ls = QSizePolicy::Fixed;
    }

    // Are we off the bottom screen?
    if(p.y() >= (mBindWidget->topLevelWidget()->height() - height()))
    {
        levelVariant = "bottom";
        p.setY((centerOfBinded.y() - mBindWidget->height()) - height());
    }
    else
    {
        // Nope, show as we should
        levelVariant = "top";
    }

    // Only reset the styles if something has changed.
    if((property("direction") != directionVariant)
       || (property("level") != levelVariant))
    {
        ui->hsTopRight->changeSize(0, 0, rs);
        ui->hsTopLeft->changeSize(0, 0, ls);
        ui->hsBottomRight->changeSize(0, 0, rs);
        ui->hsBottomLeft->changeSize(0, 0, ls);
        setProperty("direction", directionVariant);
        setProperty("level", levelVariant);
        setStyleSheet(this->styleSheet());

        // Tell the content which way we are pointing just in case it needs to know.
        if(mContent)
        {
            mContent->setProperty("direction", directionVariant);
            mContent->setProperty("level", levelVariant);
            mContent->setStyleSheet(mContent->styleSheet());
        }
    }

    ui->carrotTop->setVisible(levelVariant == "top");
    ui->carrotBottom->setVisible(levelVariant == "bottom");

    move(p);
    setUpdatesEnabled(true);
}


QString& OriginCallout::getElementStyleSheet()
{
	if(mPrivateStyleSheet.length() == 0)
	{
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
	}
	return mPrivateStyleSheet;
}


const QString OriginCallout::formatTitleForBlue(const QString& blueText, const QString& text)
{
    return QString("<html><head><meta name=\"qrichtext\" content=\"1\" /><style type=\"text/css\">\n"
        "body { margin:0; padding:0; color: #2896CE; white-space: pre-wrap; }\n"
        "</style></head><body>%1</body></html>").arg(blueText) + text;
}


OriginCallout* OriginCallout::infoTipTemplate(QWidget* bindObj, const QString& text)
{
    OriginLabel* label = new OriginLabel();
    label->setSpecialColorStyle(OriginLabel::Color_White);
    label->setAlignment(Qt::AlignCenter);
    const int labelWidth = label->fontMetrics().boundingRect(text).width();
    if(bindObj && bindObj->topLevelWidget() && labelWidth > bindObj->topLevelWidget()->width())
    {
        label->setWordWrap(true);
        label->setMaximumWidth(280);
    }
    label->setText(text);
    return infoTipTemplate(bindObj, label);
}


OriginCallout* OriginCallout::infoTipTemplate(QWidget* bindObj, QWidget* content)
{
    OriginCallout* callout = new OriginCallout(bindObj, Callout_Grey);
    callout->setContent(content);
    return callout;
}


OriginCallout* OriginCallout::calloutIconTemplate(const QString& bindObjName, const QString& blueTitle, const QString& title, QWidget* content)
{
    OriginCalloutIconTemplate* calloutIconTemp = new OriginCalloutIconTemplate();
    calloutIconTemp->setTitle(formatTitleForBlue(blueTitle, title));
    calloutIconTemp->setContent(content);
    OriginCallout* callout = new OriginCallout(bindObjName, Callout_BlueBorder);
    callout->setContent(calloutIconTemp);
    connect(calloutIconTemp, SIGNAL(closeClicked()), callout, SLOT(reject()));
    return callout;
}


OriginCallout* OriginCallout::calloutIconTemplate(const QString& bindObjName, const QString& blueTitle, const QString& title, QList<UIToolkit::OriginCalloutIconTemplateContentLine*> contentLines)
{
    UIToolkit::OriginCalloutIconTemplate* calloutIconTemp = new UIToolkit::OriginCalloutIconTemplate();
    calloutIconTemp->setTitle(formatTitleForBlue(blueTitle, title));
    calloutIconTemp->setContent(contentLines);
    UIToolkit::OriginCallout* callout = new UIToolkit::OriginCallout(bindObjName, Callout_BlueBorder);
    callout->setContent(calloutIconTemp);
    connect(calloutIconTemp, SIGNAL(closeClicked()), callout, SLOT(reject()));
    return callout;
}


OriginCallout* OriginCallout::calloutIconTemplate(QWidget* bindObj, const QString& blueTitle, const QString& title, QWidget* content)
{
    OriginCalloutIconTemplate* calloutIconTemp = new OriginCalloutIconTemplate();
    calloutIconTemp->setTitle(formatTitleForBlue(blueTitle, title));
    calloutIconTemp->setContent(content);
    OriginCallout* callout = new OriginCallout(bindObj, Callout_BlueBorder);
    callout->setContent(calloutIconTemp);
    connect(calloutIconTemp, SIGNAL(closeClicked()), callout, SLOT(reject()));
    return callout;
}


OriginCallout* OriginCallout::calloutIconTemplate(QWidget* bindObj, const QString& blueTitle, const QString& title, QList<UIToolkit::OriginCalloutIconTemplateContentLine*> contentLines)
{
    UIToolkit::OriginCalloutIconTemplate* calloutIconTemp = new UIToolkit::OriginCalloutIconTemplate();
    calloutIconTemp->setTitle(formatTitleForBlue(blueTitle, title));
    calloutIconTemp->setContent(contentLines);
    UIToolkit::OriginCallout* callout = new UIToolkit::OriginCallout(bindObj, Callout_BlueBorder);
    callout->setContent(calloutIconTemp);
    connect(calloutIconTemp, SIGNAL(closeClicked()), callout, SLOT(reject()));
    return callout;
}

}
}