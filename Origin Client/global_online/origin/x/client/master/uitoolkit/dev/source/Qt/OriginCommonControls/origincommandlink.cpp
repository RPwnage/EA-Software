
#include "origincommandlink.h"
#include "ui_origincommandlink.h"

#include <qstylepainter.h>
#include <qstyleoption.h>
#include <qtextdocument.h>
#include <qtextlayout.h>
#include <qcolor.h>
#include <qfont.h>
#include <qmath.h>

namespace Origin {
    namespace UIToolkit {
QString OriginCommandLink::mPrivateStyleSheet = QString("");


OriginCommandLink::OriginCommandLink(QWidget *parent)
    : QPushButton(parent), ui(new Ui::OriginCommandLink)
{
    init();
}

OriginCommandLink::OriginCommandLink(const QString &text, QWidget *parent)
    : QPushButton(parent), ui(new Ui::OriginCommandLink)
{
    init();
    setText(text);
}

OriginCommandLink::OriginCommandLink(const QString &text, const QString &description, QWidget *parent)
    : QPushButton(parent), ui(new Ui::OriginCommandLink)
{
    init();
    setText(text);
    setDescription(description);
}

void OriginCommandLink::init()
{
    ui->setupUi(this);

    setOriginElementName("OriginCommandLink");
    m_children.append(this);

    setAttribute(Qt::WA_TranslucentBackground);

    this->fixedHeight = false;
    this->text = "";
    this->description = "";
}

OriginCommandLink::~OriginCommandLink()
{
    this->removeElementFromChildList(this);
    delete ui;
    ui = NULL;
}

QFont OriginCommandLink::titleFont() const
{
    return ui->lblText->font();
}

QFont OriginCommandLink::descriptionFont() const
{
    return ui->lblDescription->font();
}

bool OriginCommandLink::event(QEvent *e)
{
    OriginUIBase::event(e);
    return QPushButton::event(e);
}

QSize OriginCommandLink::sizeHint() const
{
    QSize size(this->width(), getWidgetHeight());
    return size;
}

int OriginCommandLink::getWidgetHeight() const
{
    int minHeight = 0;

    const int iconHeight = 26; // height of icon + layout spacing
    const int textFieldWidth = 334;

    // Get the height of the title text
    QFontMetrics fmTitle(titleFont());
    QRect rectTitle = fmTitle.boundingRect(0,0, textFieldWidth, 32000, Qt::AlignLeft|Qt::AlignTop, this->getText());

    // Min height is the icon or the title text - whichever is greater
    minHeight = qMax(rectTitle.height(), iconHeight);

    // If there is a description - add that too
    if (!this->getDescription().isEmpty())
    {
        QFontMetrics fmDesc(descriptionFont());
        QRect rectDesc = fmDesc.boundingRect(0,0, textFieldWidth, 32000, Qt::AlignLeft|Qt::AlignTop|Qt::TextWordWrap, this->getDescription());

        minHeight += rectDesc.height();
    }

    int marginTop, marginBottom;
    this->layout()->getContentsMargins(NULL, &marginTop, NULL, &marginBottom);

    minHeight += marginTop + marginBottom;

    return minHeight;
}

void OriginCommandLink::setFixedHeightProp(bool aFixedHeight)
{
    fixedHeight = aFixedHeight;
    updateFixedHeight();
}

void OriginCommandLink::updateFixedHeight()
{
    if (fixedHeight)
    {
        this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        int h = this->getWidgetHeight();
        this->setMinimumHeight(h);
        this->setMaximumHeight(h);
    }
    else
    {
        this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);
        this->setMinimumHeight(0);
        this->setMaximumHeight(16777215);
    }
    updateGeometry();
    update();
}

void OriginCommandLink::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QPushButton::paintEvent(event);
}

void OriginCommandLink::setText(const QString &aText)
{
    text = aText;
    ui->lblText->setText(text);
    updateFixedHeight();
    adjustSize();
}

void OriginCommandLink::setDescription(const QString &aDescription)
{
    description = aDescription;
    ui->lblDescription->setText(description);
    updateFixedHeight();
    adjustSize();
}

QString& OriginCommandLink::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

void OriginCommandLink::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginCommandLink::getCurrentLang()
{
     return OriginUIBase::getCurrentLang();
}

void OriginCommandLink::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}
}
}