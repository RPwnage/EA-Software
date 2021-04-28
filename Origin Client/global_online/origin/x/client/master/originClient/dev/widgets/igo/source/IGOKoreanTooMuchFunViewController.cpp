//
//  IGOKoreanTooMuchFunViewController.cpp
//  originClient
//
//  Created by Frederic Meraud on 3/11/14.
//  Copyright (c) 2014 Electronic Arts. All rights reserved.
//

#include "IGOKoreanTooMuchFunViewController.h"

#include "services/debug/DebugService.h"
#include "services/settings/SettingsManager.h"


namespace Origin
{
namespace Client
{

IGOKoreanTooMuchFunViewController::IGOKoreanTooMuchFunViewController()
    : mTimer(NULL)
{
    QString originCountry = Origin::Services::readSetting(Origin::Services::SETTING_GeoCountry);
    if (originCountry.compare("ko", Qt::CaseInsensitive) == 0 || originCountry.compare("kr", Qt::CaseInsensitive) == 0)
    {
        mTimer = new QTimer(this);
        
        // set the popup warning
        ORIGIN_VERIFY_CONNECT(mTimer, SIGNAL(timeout()), this, SLOT(showWarning()));
        
        // set up how we delete the timer
        mTimerStart = QDateTime::currentDateTime();
        
        // get and set the period
        QString KGLstring = Origin::Services::readSetting(Origin::Services::SETTING_KGLTimer);
        int KGLseconds = KGLstring.toInt();
        if (KGLseconds < 10)    // default to 1 hour if not set, or set to less than the minimum
            KGLseconds = 3600;
        mTimer->start(KGLseconds * 1000);
    }
}
    
IGOKoreanTooMuchFunViewController::~IGOKoreanTooMuchFunViewController()
{
    delete mTimer;
}

void IGOKoreanTooMuchFunViewController::showWarning()
{
    if (mTimer)
    {
        QDateTime now = QDateTime::currentDateTime();
        
        // The setting is for QA to speed up the frequency of the message but
        // the actual use is always once per hour, so the text is only set up
        // for hours.
        QString intervalStr = Origin::Services::readSetting(Origin::Services::SETTING_KGLTimer);
        int interval = intervalStr.toInt();
        
        // We report whole hours; partial hours don't count.
        int elapsed = mTimerStart.secsTo(now) / interval;
        
        if (elapsed > 0)
        {
            QString title = tr("ebisu_client_take_a_break");
            QString hours;
            
            if (elapsed == 1)
            {
                hours = tr("ebisu_client_interval_hour_singular").arg(elapsed);
            }
            else
            {
                hours = tr("ebisu_client_interval_hour_plural").arg(elapsed);
            }
            
            QString message = tr("ebisu_client_take_a_break_time").arg(hours);
            emit Origin::Engine::IGOController::instance()->igoShowToast("POPUP_OIG_KOREAN_WARNING", title, message);
        }
    }
}

} // Client
} // Origin
