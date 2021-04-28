#include "originsurveywidget.h"
#include "ui_originsurveywidget.h"
#include <QResizeEvent>

namespace Origin {
    namespace UIToolkit {
QString OriginSurveyWidget::mPrivateStyleSheet = QString("");
  
OriginSurveyWidget::OriginSurveyWidget(QWidget* parent /* = NULL */)
: QWidget(parent)
, mResult(-1)
{
    ui = new Ui::OriginSurveyWidget();
    ui->setupUi(this);
    setOriginElementName("OriginSurveyWidget");
    m_children.append(this);

    // connect all the toggles
    connect(ui->zeroRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->oneRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->twoRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->threeRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->fourRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->fiveRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->sixRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->sevenRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->eightRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->nineRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
    connect(ui->tenRB, SIGNAL(toggled(bool)), this, SLOT(btnToggled(bool)));
}

OriginSurveyWidget::~OriginSurveyWidget()
{
    this->removeElementFromChildList(this);
}

void OriginSurveyWidget::setResult(short result)
{
    switch (result)
    {
        case 0:
            ui->zeroRB->setChecked(true);
            break;
        case 1:
            ui->oneRB->setChecked(true);
            break;
        case 2:
            ui->twoRB->setChecked(true);
            break;
        case 3:
            ui->threeRB->setChecked(true);
            break;
        case 4:
            ui->fourRB->setChecked(true);
            break;
        case 5:
            ui->fiveRB->setChecked(true);
            break;
        case 6:
            ui->sixRB->setChecked(true);
            break;
        case 7:
            ui->sevenRB->setChecked(true);
            break;
        case 8:
            ui->eightRB->setChecked(true);
            break;
        case 9:
            ui->nineRB->setChecked(true);
            break;
        case 10:
            ui->tenRB->setChecked(true);
            break;
        default:
            ui->zeroRB->setChecked(false);
            ui->oneRB->setChecked(false);
            ui->twoRB->setChecked(false);
            ui->threeRB->setChecked(false);
            ui->fourRB->setChecked(false);
            ui->fiveRB->setChecked(false);
            ui->sixRB->setChecked(false);
            ui->sevenRB->setChecked(false);
            ui->eightRB->setChecked(false);
            ui->nineRB->setChecked(false);
            ui->tenRB->setChecked(false);
            mResult = -1;
            break;
    }
}

short OriginSurveyWidget::getResult() const
{
    return mResult;
}

void OriginSurveyWidget::btnToggled(bool currentSetting)
{
    if (!currentSetting) {
        mResult = -1;
    }
    else {
        // otherwise it got turned on
        OriginRadioButton* btn = dynamic_cast<OriginRadioButton*>(QObject::sender());
        if (btn == ui->zeroRB) {
            mResult = 0;
        } else if (btn == ui->oneRB) {
            mResult = 1;
        } else if (btn == ui->twoRB) {
            mResult = 2;
        } else if (btn == ui->threeRB) {
            mResult = 3;
        } else if (btn == ui->fourRB) {
            mResult = 4;
        } else if (btn == ui->fiveRB) {
            mResult = 5;
        } else if (btn == ui->sixRB) {
            mResult = 6;
        } else if (btn == ui->sevenRB) {
            mResult = 7;
        } else if (btn == ui->eightRB) {
            mResult = 8;
        } else if (btn == ui->nineRB) {
            mResult = 9;
        } else {
            mResult = 10;
        }
    }
    emit resultChanged(mResult);
}

QString& OriginSurveyWidget::getElementStyleSheet()
{
    if (mPrivateStyleSheet.isEmpty() && !m_styleSheetPath.isEmpty())
    {
        mPrivateStyleSheet = loadPrivateStyleSheet(m_styleSheetPath, m_OriginElementName, m_OriginSubElementName);
    }
    return mPrivateStyleSheet;
}

bool OriginSurveyWidget::event(QEvent* event)
{
    OriginUIBase::event(event);
    return QWidget::event(event);
}

void OriginSurveyWidget::resizeEvent(QResizeEvent* event)
{
    const int maxWidth = (event->size().width() - 20) / 2;
    ui->notLikelyLabel->setFixedWidth(maxWidth);
    ui->extremelyLikelyLabel->setFixedWidth(maxWidth);
    QWidget::resizeEvent(event);
}

void OriginSurveyWidget::paintEvent(QPaintEvent* event)
{
    OriginUIBase::paintEvent(event);
    QWidget::paintEvent(event);
}

void OriginSurveyWidget::prepForStyleReload()
{
    mPrivateStyleSheet = QString("");
}

}

}