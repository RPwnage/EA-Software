#include "originmsgbox.h"
#include <QKeyEvent>

#include <QDialogButtonBox>
#include <QPushButton>
#include "originpushbutton.h"
#include "ui_OriginMessageBox.h"

#define max(a,b) ((a)>(b)?(a):(b))

namespace Origin {
    namespace UIToolkit {
QString OriginMsgBox::mPrivateStyleSheet = QString("");


OriginMsgBox::OriginMsgBox(QWidget* content, QWidget *parent) 
: QFrame(parent), ui(new Ui::OriginMessageBox)
, mIconType(NoIcon)
{
    init();
}


OriginMsgBox::OriginMsgBox(IconType icon, const QString &title, const QString &text,
                         QWidget* content, QWidget *parent)
: QFrame(parent), ui(new Ui::OriginMessageBox)
{
    init(icon, title, text, content);
}


OriginMsgBox::~OriginMsgBox()
{
    // Don't trust anyone to clean up their own memory.
    removeContent();
    this->removeElementFromChildList(this);
    delete ui;
    ui = NULL;
}


void OriginMsgBox::init(IconType icon, const QString &title, 
                       const QString &text, QWidget* content)
{
    mContent = NULL;

    ui->setupUi(this);
    setOriginElementName("OriginMessageBox");
    m_children.append(this);

    setup(icon, title, text, content);
}


void OriginMsgBox::setContent(QWidget* content)
{
    if(content)
    {
        removeContent();
        content->adjustSize();
        mContent = content;
        ui->mainLayout->insertWidget(2, mContent, 0);
        layoutMsgBox();
    }
}


void OriginMsgBox::removeContent()
{
    if(mContent)
    {
        ui->mainLayout->removeWidget(mContent);
        delete mContent;
        mContent = NULL;
        layoutMsgBox();
    }
}


QString& OriginMsgBox::getElementStyleSheet()
{
    if (mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginMsgBox::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}


bool OriginMsgBox::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QFrame::event(event);
}


void OriginMsgBox::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QFrame::paintEvent(event);
}


void OriginMsgBox::setIcon(IconType icon)
{
    mIconType = icon;
    switch (mIconType)
    {
    case Error:
        ui->icon->setProperty("style","error");
        ui->verticalLayout->insertWidget(0, ui->icon, 0);
        break;

    case Info:
        ui->icon->setProperty("style","info");
        ui->verticalLayout->insertWidget(0, ui->icon, 0);
        break;

    case Notice:
        ui->icon->setProperty("style","notice");
        ui->verticalLayout->insertWidget(0, ui->icon, 0);
        break;

    case Success:
        ui->icon->setProperty("style","validated");
        ui->verticalLayout->insertWidget(0, ui->icon, 0);
        break;

    default:
    case NoIcon:
        ui->icon->setProperty("style", QVariant::Invalid);
        ui->verticalLayout->removeWidget(ui->icon);
        break;
    }

    ui->icon->setStyle(QApplication::style());
}

OriginLabel* OriginMsgBox::getTitleLabel()
{
    return ui->lblMsgBoxTitle;
}

QLabel* OriginMsgBox::getTextLabel()
{
    return ui->lblMsgBoxText;
}


void OriginMsgBox::setText(const QString &text)
{
    ui->lblMsgBoxText->setText(text);
    ui->lblMsgBoxText->adjustSize();
    if(ui->lblMsgBoxText->text().isEmpty() || ui->lblMsgBoxText->text() == "")
    {
        ui->mainLayout->removeWidget(ui->lblMsgBoxText);
    }
    else 
    {
        ui->mainLayout->insertWidget(1, ui->lblMsgBoxText, 1);
    }
    layoutMsgBox();
}


const QString OriginMsgBox::getText() const
{
    return ui->lblMsgBoxText->text();
}


void OriginMsgBox::setTitle(const QString& title)
{
    ui->lblMsgBoxTitle->setText(title);
    ui->lblMsgBoxTitle->setVisible(!title.isEmpty());
    layoutMsgBox();
}

const QString OriginMsgBox::getTitle() const
{
    return ui->lblMsgBoxTitle->text();
}

void OriginMsgBox::retranslate()
{
    ui->retranslateUi(this);
    // TODO - we need the strings to be down here so that we can retranslate!
}


void OriginMsgBox::changeEvent( QEvent* event )
{
    if ( event && event->type() == QEvent::LanguageChange )
        retranslate();
}


void OriginMsgBox::setup(IconType icon, const QString &title, const QString &text, QWidget* content)
{
    setTitle(title);
    setText(text);
    setIcon(icon);
    setContent(content);
}

void OriginMsgBox::layoutMsgBox()
{
    if (parentWidget())
    {
        bool hasIcon = (mIconType == NoIcon) ? false : true;
        bool hasText = (ui->lblMsgBoxText->text().isEmpty() || ui->lblMsgBoxText->text() == "") ? false : true;
        bool hasTitle = (ui->lblMsgBoxTitle->text().isEmpty() || ui->lblMsgBoxTitle->text() == "") ? false : true;
        bool hasContent = mContent ? true : false;

        int marginLeft, marginTop, marginRight, marginBottom;
        ui->mainLayout->getContentsMargins(&marginLeft, &marginTop, &marginRight, &marginBottom);

        int availableWidth = parentWidget()->width() - (marginLeft + marginRight);

        int numRows = 0;
        int calcHeight = marginTop + marginBottom;

        // Calculate the height of the icon
        int iconHeight = 0;
        if (hasIcon)
        {
            iconHeight = ui->icon->height();
        }

        // Calculate the height of the title
        int titleHeight = 0;
        if (hasTitle)
        {
            QFontMetrics fm(ui->lblMsgBoxTitle->font());
            QRect r = fm.boundingRect(0,0, availableWidth - (hasIcon ? (ui->icon->width() + ui->horizontalLayout->spacing()) : 0), 32000, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, ui->lblMsgBoxTitle->text());

            titleHeight = r.height();
        }

        // Add the icon / title combination
        if (hasIcon || hasTitle)
        {
            calcHeight += max(iconHeight, titleHeight);
            ++numRows;
        }

        // Calculate the height of the text
        if (hasText)
        {
            QFontMetrics fm(ui->lblMsgBoxText->font());
            QRect r = fm.boundingRect(0,0, availableWidth, 32000, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, ui->lblMsgBoxText->text());

            int textHeight = r.height();
            calcHeight += textHeight;

            ++numRows;
        }

        // Calculate the height of the content
        if (hasContent)
        {
            int contentHeight = mContent->height();
            calcHeight += contentHeight;
            ++numRows;
        }

        // Add spacing between rows
        if (numRows > 1)
        {
            calcHeight += ui->mainLayout->spacing() * (numRows - 1);
        }

        // As per Design spec - no short dialogs!!
        if (calcHeight < 200)
        {
            calcHeight = 200;
        }

        // Set our final calculated height
        if (height() != calcHeight)
        {
            setMinimumHeight(calcHeight);
            setMaximumHeight(calcHeight);
        }
    }
}

}
}