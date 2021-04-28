/////////////////////////////////////////////////////////////////////////////
// NetPromoterWidget.cpp
//
// Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#include "NetPromoterWidget.h"
#include "ui_NetPromoterWidget.h"

namespace Origin
{

namespace Client 
{

NetPromoterWidget::NetPromoterWidget(NetPromoterWidgetType questionType, QWidget* parent /* = NULL */)
    : QWidget(parent)
    , ui(new Ui::NetPromoterWidget())
    , mType(questionType)
{
    ui->setupUi(this);
    connect(ui->originSurveyWidget, SIGNAL(resultChanged(int)), this, SLOT(surveyChanged(int)));

    ui->feedBackContainer->setVisible(questionType == TWO_QUESTION);
    translateUi();
}

void NetPromoterWidget::surveyChanged(int newNum)
{
    if (newNum == -1)
        emit surveySelected(false);
    else 
        emit surveySelected(true);
}

unsigned short NetPromoterWidget::surveyNumber() const
{
    return ui->originSurveyWidget->getResult();

}

QString NetPromoterWidget::surveyText() const
{
    if (mType == TWO_QUESTION) 
    {
        return ui->moreInput->getMultiLineWidget()->toPlainText();
    }
    return QString();
}

bool NetPromoterWidget::canContactUser() const
{
    return (ui && ui->chkContactUser) ? ui->chkContactUser->isChecked() : false;
}

void NetPromoterWidget::changeEvent( QEvent* event )
{
    switch(event->type())
    {
    case QEvent::LanguageChange:
        translateUi();
        break;

    default:
        break;
    }
    QWidget::changeEvent( event );
}

void NetPromoterWidget::translateUi()
{
    ui->retranslateUi(this);
    ui->likelyToRecommendQuestion->setText(tr("ebisu_client_recommend_origin").arg(tr("application_name")));
}

} // client

}
