#include "IGOTooltipSliderWidget.h"
#include "ui_IGOTooltipSliderWidget.h"
#include "services/debug/DebugService.h"
#include "services/qt/QtUtil.h"

#include <QPainter.h>
#include <QMouseEvent>

namespace Origin
{
namespace Client
{

IGOTooltipSliderWidget::IGOTooltipSliderWidget(QWidget* parent)
: QWidget(parent, Qt::FramelessWindowHint | Qt::Tool)
, ui(new Ui::IGOTooltipSliderWidget)
{
    ui->setupUi(this);
    setSliderValue(191);
    setAttribute(Qt::WA_TranslucentBackground);
    ui->originSlider->installEventFilter(this);
    ORIGIN_VERIFY_CONNECT(ui->originSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged()));
    ORIGIN_VERIFY_CONNECT(ui->originSlider, SIGNAL(sliderReleased()), this, SIGNAL(mouseReleased()));
    adjustSize();
    ui->content->setMask(Origin::Services::roundedRect(ui->content->rect(), 6, false));
}


IGOTooltipSliderWidget::~IGOTooltipSliderWidget()
{
    if(ui)
    {
        delete ui;
        ui = NULL;
    }
}


void IGOTooltipSliderWidget::setSliderValue(const int& val)
{
    ORIGIN_VERIFY_DISCONNECT(ui->originSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged()));
    ui->originSlider->setValue(val);
    const int percent = ((float(ui->originSlider->value()) / float(ui->originSlider->maximum())) * 100.0f);
    ui->lblPercent->setText(tr("ebisu_client_percent_notation").arg(QString::number(percent)));
    ORIGIN_VERIFY_CONNECT(ui->originSlider, SIGNAL(valueChanged(int)), this, SLOT(onSliderChanged()));
}


int IGOTooltipSliderWidget::sliderValue() const
{
    return ui->originSlider->value();
}


void IGOTooltipSliderWidget::onSliderChanged()
{
    const int percent = ((float(ui->originSlider->value()) / float(ui->originSlider->maximum())) * 100.0f);
    ui->lblPercent->setText(tr("ebisu_client_percent_notation").arg(QString::number(percent)));
    emit sliderChanged(percent);
}


void IGOTooltipSliderWidget::paintEvent(QPaintEvent*)
{
    QStyleOption opt;
    opt.init(this);
    QPainter p(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);
}


bool IGOTooltipSliderWidget::event(QEvent *e)
{
    QEvent::Type etype = e->type();
    switch(etype)
    {
    case QEvent::Enter:
    case QEvent::FocusIn:
    case QEvent::HoverEnter:
        emit active();
        break;
    case QEvent::KeyRelease:
    case QEvent::KeyPress:
    //case QEvent::FocusOut:
    case QEvent::HoverLeave:
    //case QEvent::Hide:
    case QEvent::Leave:
    case QEvent::DeferredDelete:
    case QEvent::Destroy:
        emit requestVisibilityChange(false);
        break;
    default:
        break;
    }

    return QWidget::event(e);
}


bool IGOTooltipSliderWidget::eventFilter(QObject* obj, QEvent* e)
{
    QEvent::Type etype = e->type();
    switch(etype)
    {
    case QEvent::MouseButtonPress:
        {
            QMouseEvent* mouseEvent = dynamic_cast<QMouseEvent*>(e);
            if(obj == ui->originSlider && mouseEvent && mouseEvent->button() == Qt::LeftButton)
            {
                // Need to put this in the event filter so the OriginSlider can handle the event before we emit this
                // signal for the igoqtcontainer
                emit mousePressed();
            }
        }

        break;
    default:
        break;
    }

    return QWidget::eventFilter(obj, e);
}
    
} // Client
} // Origin
