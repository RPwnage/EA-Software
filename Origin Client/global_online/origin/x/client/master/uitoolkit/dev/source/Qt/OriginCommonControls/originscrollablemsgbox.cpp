#include "originscrollablemsgbox.h"
#include "ui_originscrollablemsgbox.h"
#include "originpushbutton.h"
#include <QDialogButtonBox>
#include <QScrollBar>

namespace Origin {
namespace UIToolkit {
QString OriginScrollableMsgBox::mPrivateStyleSheet = QString("");

namespace 
{
    const int MinimumWidth = 450;
    const int MarginSize  = 30;
    const int HalfTheMarginSize  = MarginSize / 2;
    const int MinimumMsgBoxWidth = MinimumWidth - HalfTheMarginSize;
}

OriginScrollableMsgBox::OriginScrollableMsgBox(IconType icon, const QString& title, const QString& text, QWidget* parent)
: QWidget(parent)
, ui(new Ui::OriginScrollableMsgBox)
, mContent(NULL)
{
    ui->setupUi(this);
    setOriginElementName("OriginScrollableMsgBox");
    m_children.append(this);
    ui->msgbox->setAttribute(Qt::WA_NoSystemBackground, true);
    ui->msgbox->setAutoFillBackground(false);
    ui->scrollArea->horizontalScrollBar()->installEventFilter(this);

    connect(ui->scrollArea->verticalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(adjustScrollAreaHeight(int,int)));

    setup(icon, title, text);
    // By default - don't have a dynamic width.
    setHasDynamicWidth(false);
}


OriginScrollableMsgBox::~OriginScrollableMsgBox()
{
    // Don't trust anyone to clean up their own memory.
    removeContent();
    this->removeElementFromChildList(this);
    delete ui;
    ui = NULL;
}


void OriginScrollableMsgBox::adjustScrollAreaHeight(int min, int max)
{
    const int sizePlusMax = ui->scrollArea->height() + max;
    if(max >= 0 && ui->scrollArea->size().height() <= 400)
    {
        int adjustToSize = sizePlusMax;
        if(sizePlusMax >= 400)
        {
            adjustToSize = 400;
        }
        else if(sizePlusMax <= 200)
            adjustToSize = 200;

        ui->scrollArea->setMinimumHeight(adjustToSize);
    }
}


void OriginScrollableMsgBox::adjustToScrollAreaWidth(int min, int max)
{
    // We need to adjust the size of the window base on what will fit and 
    // make it as small as we can

    // Get the width of everything that matters.
    const int contentWidth = mContent ? mContent->width() : 0;
    const int msgBoxWidth = ui->msgbox->width();
    const int sizePlusMax = ui->scrollArea->width() + max;
    ui->buttonBox->adjustSize();
    ui->cornerContent->adjustSize();
    ui->exitInfo->adjustSize();
    const int exitInfoWidth = ui->exitInfo->width();
    
    // Check the size of the content first. If it's bigger than the message box and default
    // Then we need to adjust to that size. And so on with the msg box, 
    // exit info, scroll size, and default size.
    if(contentWidth >= MinimumWidth && contentWidth >= msgBoxWidth)
    {
        ui->msgbox->setMinimumWidth(0);
        setMinimumWidth(contentWidth);
    }
    else if(msgBoxWidth > MinimumWidth && msgBoxWidth > exitInfoWidth)
    {
        ui->msgbox->setMinimumWidth(msgBoxWidth);
        setMinimumWidth(sizePlusMax);
    }
    else if(exitInfoWidth > MinimumWidth && exitInfoWidth > sizePlusMax)
    {
        ui->msgbox->setMinimumWidth(0);
        setMinimumWidth(exitInfoWidth);
    }
    else if(sizePlusMax >= MinimumWidth)
    {
        ui->msgbox->setMinimumWidth(0);
        setMinimumWidth(sizePlusMax);
    }
    else
    {
        ui->msgbox->setMinimumWidth(MinimumMsgBoxWidth);
        setMinimumWidth(MinimumWidth);
    }
}


void OriginScrollableMsgBox::setContent(QWidget* content)
{
    if(content)
    {
        removeContent();
        mContent = content;
        //if(mContent)
        //{
        //    QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        //    sizePolicy1.setHorizontalStretch(0);
        //    sizePolicy1.setVerticalStretch(2);
        //    sizePolicy1.setHeightForWidth(mContent->sizePolicy().hasHeightForWidth());
        //    mContent->setSizePolicy(sizePolicy1);
        //}
        adjustForText();
        ui->gridLayout->addWidget(mContent, 2, 0, 1, 2);
    }
}


void OriginScrollableMsgBox::removeContent()
{
    if(mContent)
    {
        ui->gridLayout->removeWidget(mContent);
        delete mContent;
        mContent = NULL;
    }
}


QString& OriginScrollableMsgBox::getElementStyleSheet()
{
    if (mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginScrollableMsgBox::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}


bool OriginScrollableMsgBox::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}


bool OriginScrollableMsgBox::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type())
    {
    case QEvent::Wheel:
        // Don't scroll horizontally. We don't want to see the margin under the scroll bar.
        if(obj == ui->scrollArea->horizontalScrollBar())
        {
            event->ignore();
            return true;
        }
        break;
    default:
        break;
    }
    return QWidget::eventFilter(obj, event);
}


void OriginScrollableMsgBox::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}


void OriginScrollableMsgBox::setIcon(IconType icon)
{
    switch (icon)
    {
    case Error:
        ui->icon->setProperty("style","error");
        ui->gridLayout->addWidget(ui->icon, 0, 0);
        break;

    case Info:
        ui->icon->setProperty("style","info");
        ui->gridLayout->addWidget(ui->icon, 0, 0);
        break;

    case Notice:
        ui->icon->setProperty("style","notice");
        ui->gridLayout->addWidget(ui->icon, 0, 0);
        break;

    case Success:
        ui->icon->setProperty("style","validated");
        ui->gridLayout->addWidget(ui->icon, 0, 0);
        break;

    default:
    case NoIcon:
        ui->icon->setProperty("style", QVariant::Invalid);
        ui->gridLayout->removeWidget(ui->icon);
        break;
    }

    ui->icon->setStyle(QApplication::style());
}


QLabel* OriginScrollableMsgBox::getTextLabel()
{
    return ui->lblMsgBoxText;
}


void OriginScrollableMsgBox::setText(const QString &text)
{
    ui->lblMsgBoxText->setText(text);
    adjustForText();
}


const QString OriginScrollableMsgBox::getText() const
{
    return ui->lblMsgBoxText->text();
}


void OriginScrollableMsgBox::setTitle(const QString& title)
{
    ui->lblMsgBoxTitle->setText(title);
    ui->lblMsgBoxTitle->setVisible(!title.isEmpty());
}


const QString OriginScrollableMsgBox::getTitle() const
{
    return ui->lblMsgBoxTitle->text();
}


void OriginScrollableMsgBox::setup(IconType icon, const QString& title, const QString& text)
{
    setTitle(title);
    setText(text);
    setIcon(icon);
}


void OriginScrollableMsgBox::setCornerContent(QWidget* content)
{
    if(content && ui)
    {
        delete ui->cornerContent;
        ui->cornerContent = content;
        ui->exitLayout->insertWidget(0, ui->cornerContent);
        ui->cornerContent->show();
    }
}


QWidget* OriginScrollableMsgBox::getCornerContent() const
{
    return ui->cornerContent;
}


void OriginScrollableMsgBox::addButtons(QDialogButtonBox::StandardButtons buttons)
{
    if(ui && ui->buttonBox)
    {
        OriginButtonBox* bb = ui->buttonBox;
        if (buttons & QDialogButtonBox::Retry) bb->addButton(QDialogButtonBox::Retry);
        if (buttons & QDialogButtonBox::Close) bb->addButton(QDialogButtonBox::Close);
        if (buttons & QDialogButtonBox::Cancel) bb->addButton(QDialogButtonBox::Cancel);
        if (buttons & QDialogButtonBox::Ok) bb->addButton(QDialogButtonBox::Ok);
        if (buttons & QDialogButtonBox::Yes) bb->addButton(QDialogButtonBox::Yes);
        if (buttons & QDialogButtonBox::No) bb->addButton(QDialogButtonBox::No);

        if(buttons == QDialogButtonBox::NoButton)
        {
            bb->hide();
            ui->cornerContent->hide();
            ui->exitInfo->setProperty("style", QVariant::Invalid);
            ui->exitInfo->setStyle(QApplication::style());
            // Change the exitInfo style to hasButton if a button is added outside of here.
            connect(bb, SIGNAL(buttonAdded()), this, SLOT(onButtonAdded()));
        }
        else
        {
            ui->exitInfo->setProperty("style","hasButtons");
            ui->exitInfo->setStyle(QApplication::style());
        }

        connect(bb, SIGNAL(clicked(QAbstractButton*)), bb, SLOT(buttonBoxClicked(QAbstractButton*)));
    }
}


OriginButtonBox* OriginScrollableMsgBox::getButtonBox() const
{
    if(ui && ui->buttonBox)
        return ui->buttonBox;
    return NULL;
}


void OriginScrollableMsgBox::onButtonAdded()
{
    ui->exitInfo->setProperty("style","hasButtons");
    ui->exitInfo->setStyle(QApplication::style());
}

OriginPushButton* OriginScrollableMsgBox::button(QDialogButtonBox::StandardButton which) const
{
    if(ui && ui->buttonBox)
        return ui->buttonBox->button(which);
    return NULL;
}

void OriginScrollableMsgBox::setButtonText(QDialogButtonBox::StandardButton which, const QString &text) 
{
    if(ui && ui->buttonBox)
    {
        ui->buttonBox->setButtonText(which, text);
    }
}

void OriginScrollableMsgBox::setDefaultButton(QDialogButtonBox::StandardButton which)
{
    if(ui && ui->buttonBox)
    {
        ui->buttonBox->setDefaultButton(which);
    }
}


void OriginScrollableMsgBox::setHasDynamicWidth(const bool& dynamicWidth)
{
    if(dynamicWidth)
    {
        ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        connect(ui->scrollArea->horizontalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(adjustToScrollAreaWidth(int,int)));
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
        ui->msgbox->setMinimumSize(MinimumMsgBoxWidth, 0);
        ui->msgbox->setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    else
    {
        ui->scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        disconnect(ui->scrollArea->horizontalScrollBar(), SIGNAL(rangeChanged(int,int)), this, SLOT(adjustToScrollAreaWidth(int,int)));
        setFixedWidth(MinimumWidth);
        // This is the size needed for the text to be completely visible
        // This hack ensures that the text isn't cut off and fixes
        // EBIBUGS-20874 
        ui->msgbox->setFixedWidth(MinimumWidth);
    }
}


void OriginScrollableMsgBox::adjustForText()
{
    if(ui->lblMsgBoxText->text() == "")
    {
        ui->gridLayout->removeWidget(ui->lblMsgBoxText);
        ui->lblMsgBoxText->setParent(NULL);
    }
    else 
    {
        if(ui->lblMsgBoxText->parentWidget() == NULL)
        {
            ui->gridLayout->addWidget(ui->lblMsgBoxText, 1, 0, 1, 2);
            adjustScrollAreaHeight(0,0);
        }
    }
}

}
}