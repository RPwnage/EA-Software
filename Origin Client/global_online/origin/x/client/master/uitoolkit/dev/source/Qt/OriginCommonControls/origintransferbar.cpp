#include <QtWidgets>
#include "origintransferbar.h"
#include "ui_origintransferbar.h"

namespace Origin {
    namespace UIToolkit {
QString OriginTransferBar::mPrivateStyleSheet = QString("");


OriginTransferBar::OriginTransferBar(QWidget *parent/*=0*/) :
    QWidget(parent), ui(new Ui::OriginTransferBar)
{
    ui->setupUi(this);

    setOriginElementName("OriginTransferBar");
    m_children.append(this);
    setAttribute(Qt::WA_TranslucentBackground);
}

OriginTransferBar::~OriginTransferBar()
{
    this->removeElementFromChildList(this);
    delete ui;
    ui = NULL;
}

QString & OriginTransferBar::getElementStyleSheet()
{
    if ( mPrivateStyleSheet.length() == 0)
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginTransferBar::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}

void OriginTransferBar::paintEvent(QPaintEvent *event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}

void OriginTransferBar::changeLang(QString newLang)
{
    OriginUIBase::changeCurrentLang(newLang);
}

QString& OriginTransferBar::getCurrentLang()
{
    return OriginUIBase::getCurrentLang();
}

void OriginTransferBar::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

// PROPERTY SETTERS

void OriginTransferBar::setState(QString const& aState)
{
    state = aState;
    ui->lblState->setText(aState);
}

void OriginTransferBar::setTitle(QString const& aTitle)
{
    title = aTitle;
    ui->lblTitle->setText(aTitle);
}

void OriginTransferBar::setCompleted(QString const& aCompleted)
{
    completed = aCompleted;
    ui->lblCompleted->setText(aCompleted);
}

void OriginTransferBar::setRate(QString const& aRate)
{
    rate = aRate;
    ui->lblRate->setText(aRate);
}

OriginProgressBar* OriginTransferBar::getProgressBar()
{
    return ui->progressBar;
}


float OriginTransferBar::getValue() const
{
    return ui->progressBar->getValue();
}

void OriginTransferBar::setTimeRemaining(QString const& aTimeRemaining)
{
    timeRemaining = aTimeRemaining;
    ui->lblTimeRemaining->setText(aTimeRemaining);
}

void OriginTransferBar::setValue(int aValue)
{
    ui->progressBar->setValue(aValue);
}

void OriginTransferBar::setValue(double aValue)
{
    ui->progressBar->setValue(aValue);
}

}
}
