/////////////////////////////////////////////////////////////////////////////
// MultiLaunchView.h
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////
#include "MultiLaunchView.h"
#include <QVBoxLayout>
#include "originradiobutton.h"
#include "services/debug/DebugService.h"

namespace Origin 
{
namespace Client
{

MultiLaunchView::MultiLaunchView(const QStringList& launchNameList, QWidget* parent)
: QWidget(parent)
, mLaunchOptionSelected("")
{
    // In our flow we wouldn't get here if the list was empty - but lets check again anyway.
    if(launchNameList.count() > 0)
    {
        mLaunchOptionSelected = launchNameList[0];

        QVBoxLayout* verticalLayout = new QVBoxLayout(this);
        verticalLayout->setSpacing(6);
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        using namespace UIToolkit;
        const int listCount = launchNameList.count();
        for(int iName = 0; iName < listCount; iName++)
        {
            OriginRadioButton* radioButton = new OriginRadioButton(this);
            radioButton->setChecked(iName == 0);
            QString name = launchNameList[iName];
            // & creates a shortcut look - to display an actual ampersand, use '&&'.
            name.replace("&", "&&");
            radioButton->setText(name);
            ORIGIN_VERIFY_CONNECT(radioButton, SIGNAL(clicked()), this, SLOT(onRadioClicked()));
            verticalLayout->addWidget(radioButton);
        }
        QSpacerItem* verticalSpacer = new QSpacerItem(20, 1, QSizePolicy::Minimum, QSizePolicy::Expanding);
        verticalLayout->addItem(verticalSpacer);
    }
}


MultiLaunchView::~MultiLaunchView()
{
}


void MultiLaunchView::onRadioClicked()
{
    using namespace Origin::UIToolkit;
    OriginRadioButton* radioButton = dynamic_cast<OriginRadioButton*>(this->sender());
    if(radioButton)
    {
        mLaunchOptionSelected = radioButton->text();
        mLaunchOptionSelected.replace("&&", "&");
    }
}

}
}