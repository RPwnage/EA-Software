#include "originbanner.h"
#include "ui_OriginBanner.h"

namespace Origin {
    namespace UIToolkit {
QString OriginBanner::mPrivateStyleSheet = QString("");


OriginBanner::OriginBanner(QWidget* parent, const IconType& iconType,
                         const BannerType& bannerType)
: QWidget(parent)
, ui(NULL)
, mIconType(IconUninitialized)
, mBannerType(BannerUninitialized)
{
    ui = new Ui::OriginBanner();
    ui->setupUi(this);
    setOriginElementName("OriginBanner");
    m_children.append(this);

    setIconType(iconType);
    setBannerType(bannerType);
    ui->lblText->sizePolicy().setHeightForWidth(true);
    ui->lblText->setTextInteractionFlags((Qt::TextInteractionFlags)(Qt::LinksAccessibleByKeyboard | Qt::LinksAccessibleByMouse));
    connect(ui->lblText, SIGNAL(linkActivated(const QString&)), this, SIGNAL(linkActivated(const QString&)));
}


OriginBanner::~OriginBanner()
{
	removeElementFromChildList(this);

    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void OriginBanner::setIconType(IconType iconType)
{
    if(mIconType != iconType && iconType != IconUninitialized)
    {
        mIconType = iconType;
        QVariant variant = QVariant::Invalid;
        switch(mIconType)
        {
        case Success:
            variant = "success";
            break;

        case Error:
            variant = "error";
            break;

        case Info:
            variant = "info";
            break;

        case Notice:
            variant = "notice";
            break;
            
        default:
            break;
        }

        ui->lblText->setProperty("iconType", mBannerType==Banner?variant:QVariant::Invalid);
        ui->lblText->setStyle(QApplication::style());
        ui->icon->setProperty("iconType", variant);
        ui->icon->setStyle(QApplication::style());
        ui->innerWidget->setProperty("iconType", variant);
        ui->innerWidget->setStyle(QApplication::style());
    }
}


void OriginBanner::setBannerType(BannerType bannerType)
{
    if(mBannerType != bannerType && bannerType != BannerUninitialized)
    {
        mBannerType = bannerType;
        QPalette pal;
        switch(mBannerType)
        {
        case Banner:
            ui->lblText->setWordWrap(true);
            ui->innerWidget->setProperty("bannerType", QVariant::Invalid);
            ui->lblText->setProperty("bannerType", QVariant::Invalid);
            ui->spacerLeft->changeSize(13, 1, QSizePolicy::Fixed);
            ui->spacerRight->changeSize(13, 1, QSizePolicy::Fixed);
            break;

        case Message:
            ui->lblText->setProperty("iconType", QVariant::Invalid);
            ui->innerWidget->setProperty("bannerType", "message");
            ui->lblText->setProperty("bannerType", "message");
            formatLabel();
            break;
            
        default:
            break;
        }
        ui->innerWidget->setStyle(QApplication::style());
        ui->lblText->setStyle(QApplication::style());
    }
}


void OriginBanner::setText(const QString& text)
{
    ui->lblText->setText(text);
    formatLabel();
}


QString OriginBanner::text() const
{
    return ui->lblText->text();
}


QString& OriginBanner::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        // mBannerType == Banner
        setOriginElementName("OriginLabel", "DialogLabel");
        mPrivateStyleSheet = loadPrivateStyleSheet(":/OriginCommonControls/OriginLabel/styleDialogLabel.qss", "OriginLabel", "DialogLabel");
        mPrivateStyleSheet.replace("Origin--UIToolkit--OriginLabel", "#lblText");

        // mBannerType == Message
        setOriginElementName("OriginLabel", "DialogTitle");
        mPrivateStyleSheet += loadPrivateStyleSheet(":/OriginCommonControls/OriginLabel/styleDialogTitle.qss", "OriginLabel", "DialogTitle");
        mPrivateStyleSheet.replace("Origin--UIToolkit--OriginLabel", "#lblText[bannerType=\"message\"]");

        setOriginElementName("OriginBanner", "");
        mPrivateStyleSheet += loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}


void OriginBanner::prepForStyleReload()
{
	mPrivateStyleSheet = QString("");
}


bool OriginBanner::event(QEvent* event)
{
	OriginUIBase::event(event);
    return QWidget::event(event);
}


void OriginBanner::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}


void OriginBanner::formatLabel()
{
    if(parentWidget() && mBannerType == Message 
        && ui->lblText->text().isEmpty() == false)
    {
        // Update the size after we set the text so we can do our calculations
        // We set the word wrap to false so we can have a lot of control in
        // setting the margin.
        // The word wrap sometimes wraps labels that could fit on one line
        ui->lblText->setWordWrap(false);
		QMetaObject::invokeMethod (this, "adjustWidgetSize", Qt::QueuedConnection, Q_ARG (QWidget*, ui->lblText));

        // Find the size of the icon and text
        int bannerWidth = (ui->icon->width() + ui->lblText->width());

        // Find the size of the parent minus the minimum margin x2 
        // for left and right
        const int parentWidth = parentWidget()->width();

        // Minimum margin size is 30px.
        static const int MIN_MARGIN = 30;

        // If it needs to go on multiple lines
        if(bannerWidth > (parentWidth-(MIN_MARGIN << 1)))
        {
            // Set the word wrap to true and update the size of the label
            ui->lblText->setWordWrap(true);
			QMetaObject::invokeMethod (this, "adjustWidgetSize", Qt::QueuedConnection, Q_ARG (QWidget*, ui->lblText));
            // Now that the size has changed, update the width of the label
            bannerWidth = (ui->icon->width() + ui->lblText->width());
        }
        
        // Calculate the remaining space and let that be the margin
        int margin = ((parentWidth - bannerWidth) >> 1);
        margin = margin > MIN_MARGIN ? margin : MIN_MARGIN;

        ui->spacerLeft->changeSize(margin, 1, QSizePolicy::Fixed);
        ui->spacerRight->changeSize(margin, 1, QSizePolicy::Fixed);
    }
}

void OriginBanner::adjustWidgetSize (QWidget* widget)
{
	widget->adjustSize();
}

}
}
