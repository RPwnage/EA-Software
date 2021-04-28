#include "OriginTab.h"
#include "ui_OriginTab.h"

#include <QMouseEvent>

namespace Origin {
namespace UIToolkit {

QString OriginTab::mPrivateStyleSheet = QString("");

OriginTabIcon::OriginTabIcon(QWidget* parent)
: QPushButton(parent)
{
    setObjectName(QString::fromUtf8("btnIcon"));
    setEnabled(true);
    setFixedSize(23, 23);
    setFocusPolicy(Qt::NoFocus);
    setFlat(true);}

OriginTabIcon::~OriginTabIcon()
{
}

void OriginTabIcon::mouseMoveEvent(QMouseEvent *e)
{
    e->ignore();
}

void OriginTabIcon::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
}

void OriginTabIcon::mouseReleaseEvent(QMouseEvent *e)
{
    e->ignore();
}


////////////////////////////////////////////////////////////////////////////////
OriginTabCloseButton::OriginTabCloseButton(QWidget* parent)
: QPushButton(parent)
, mIsMouseDown(false)
, mIsDragging(false)
, mMouseDownPos()
, mStartDragDistance(QApplication::startDragDistance())
{
    setObjectName(QString::fromUtf8("btnClose"));
    setFixedSize(16, 16);
}

OriginTabCloseButton::~OriginTabCloseButton()
{

}

void OriginTabCloseButton::mousePressEvent(QMouseEvent *e)
{
    if( !mIsMouseDown && e->button() == Qt::LeftButton )
    {
        mIsMouseDown = true;
        mIsDragging = false;
        mMouseDownPos = e->pos();

        // set the pressed image
        setProperty("state", "pressed");
        setStyle(QApplication::style());
    }
    e->ignore();
}

void OriginTabCloseButton::mouseMoveEvent(QMouseEvent *e)
{
    if( mIsMouseDown )
    {
        int dx = (e->pos() - mMouseDownPos).manhattanLength();
        if( dx > mStartDragDistance )
        {
            mIsDragging = true;
        }
    }
    e->ignore();
}

void OriginTabCloseButton::mouseReleaseEvent(QMouseEvent *e)
{
    if( mIsMouseDown && e->button() == Qt::LeftButton )
    {
        if( !mIsDragging )
        {
            click();
            e->accept(); // do not propagate
        }
        else
        {
            e->ignore();
        }

        mIsMouseDown = false;

        // reset the image
        setProperty("state", QVariant::Invalid);
        setStyle(QApplication::style());
    }
}


////////////////////////////////////////////////////////////////////////////////
OriginTab::OriginTab(QWidget* parent)
: QWidget(parent)
{
    ui = new Ui::OriginTab();
    ui->setupUi(this);
    setOriginElementName("OriginTab");
    m_children.append(this);

    ui->btnIcon->setIcon(QIcon(":/Resources/GUI/browser/default_page.png"));
    ui->lblTitle->setText(tr("ebisu_client_new_tab"));
    ui->horizontalLayout->addWidget(ui->btnClose);

    connect( ui->btnClose, SIGNAL(clicked()), this, SLOT(onCloseButtonClicked()) );
}

OriginTab::~OriginTab()
{
    removeElementFromChildList(this);
}

void OriginTab::setText(const QString& text)
{
    ui->lblTitle->setText(text);
}

void OriginTab::setIcon(const QIcon& icon)
{
    ui->btnIcon->setIcon(icon);
}

void OriginTab::setCloseButtonVisible(bool visible)
{
    ui->btnClose->setVisible(visible);
}

void OriginTab::setIconVisible(bool visible)
{
    ui->btnIcon->setVisible(visible);
}

void OriginTab::setTitleVisible(bool visible)
{
    ui->lblTitle->setVisible(visible);

    const int width = visible ? 1 : 5;
    ui->horizontalSpacer->changeSize(width,1, visible ? QSizePolicy::Fixed : QSizePolicy::Expanding);
}

OriginTabCloseButton *OriginTab::closeButton()
{
    return static_cast<OriginTabCloseButton *>(ui->btnClose);
}

OriginTabIcon* OriginTab::iconButton()
{
    return static_cast<OriginTabIcon *>(ui->btnIcon);
}

int OriginTab::labelWidth()
{
    return ui->lblTitle->width();
}

QFont OriginTab::labelFont()
{
    return ui->lblTitle->font();
}

QString OriginTab::text() const
{
    return ui->lblTitle->text();
}

QString& OriginTab::getElementStyleSheet()
{
    if(mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet += loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

void OriginTab::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

bool OriginTab::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}

void OriginTab::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}

void OriginTab::enterEvent(QEvent* event)
{
    QVariant state = property("state");
    if( state.toString() != "active" )
    {
        setProperty("state", "hover");
        setStyle(QApplication::style());
        emit enterTab();
    }

    QWidget::enterEvent(event);
}

void OriginTab::leaveEvent(QEvent* event)
{
    QVariant state = property("state");
    if( state.toString() != "active" )
    {
        setProperty("state", "normal");
        setStyle(QApplication::style());
        emit leaveTab();
    }

    QWidget::leaveEvent(event);
}

void OriginTab::onCloseButtonClicked()
{
    emit closeRequested(this);
}

}
}
