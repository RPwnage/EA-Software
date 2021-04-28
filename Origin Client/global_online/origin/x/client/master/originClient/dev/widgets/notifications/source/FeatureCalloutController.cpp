#include "FeatureCalloutController.h"
#include "services/debug/DebugService.h"
#include "services/log/LogService.h"
#include "services/settings/SettingsManager.h"
#include "TelemetryAPIDLL.h"
#include <QWidget>
#include "originlabel.h"
#include "origincallout.h"
#include "origincallouticontemplate.h"

namespace Origin
{
namespace Client
{

CalloutSetting::CalloutSetting(const Release& featureVersion, const QString& settingName, const QString& parentName, const QString& seriesName)
: mFeatureVersion(featureVersion)
, mSettingName(settingName)
, mBindedObjectName(parentName)
, mSeriesName(seriesName)
{
}


FeatureCalloutController::FeatureCalloutController()
{
    ORIGIN_LOG_ACTION << "Creating feature callout controller";
    // We have to append the settings here in because we couldn't use revertSettingsForRelease if we don't know which settings are.
    // PLEASE Do not take advantage of having which setting in here and putting setting specific code. This is just for ease of use for reverting settings.
    mCalloutSettings.append(CalloutSetting(CalloutSetting::RELEASE_9_5, Services::SETTING_CALLOUT_SOCIALUSERAREA_SHOWN.name(), "btnShowFriends", "group_chat_voip"));
    mCalloutSettings.append(CalloutSetting(CalloutSetting::RELEASE_9_5, Services::SETTING_CALLOUT_GROUPSTAB_SHOWN.name()));
    mCalloutSettings.append(CalloutSetting(CalloutSetting::RELEASE_9_5, Services::SETTING_CALLOUT_VOIPBUTTON_ONE_TO_ONE_SHOWN.name()));
    mCalloutSettings.append(CalloutSetting(CalloutSetting::RELEASE_9_5, Services::SETTING_CALLOUT_VOIPBUTTON_GROUP_SHOWN.name()));
}


FeatureCalloutController::~FeatureCalloutController()
{
    ORIGIN_LOG_ACTION << "Feature Callout: Clearing map - contained " << QString::number(mCalloutMap.count()) << " items remaining (should be 0).";
    mCalloutMap.clear();
}


CalloutSetting FeatureCalloutController::calloutSettingBySettingName(const QString& settingName)
{
    foreach(CalloutSetting iSetting, mCalloutSettings)
    {
        if(iSetting.mSettingName == settingName)
            return iSetting;
    }
    return CalloutSetting();
}


CalloutSetting FeatureCalloutController::calloutSettingByBindName(const QString& bindObjName)
{
    foreach(CalloutSetting iSetting, mCalloutSettings)
    {
        if(iSetting.mBindedObjectName == bindObjName)
            return iSetting;
    }
    return CalloutSetting();
}


void FeatureCalloutController::revertSettingsForRelease(const CalloutSetting::Release& number)
{
    // This is typically called from the "What's New" menu item. In order for us to
    // get a "settingChanged" signal generated, the setting needs to actually change.
    // For this we set each setting first to true, then false again.
    foreach(CalloutSetting s, mCalloutSettings)
    {
        if(s.mFeatureVersion == number)
        {
            writeCalloutSetting(s.mSettingName, true);
        }
    }
    foreach(CalloutSetting s, mCalloutSettings)
    {
        if(s.mFeatureVersion == number)
        {
            writeCalloutSetting(s.mSettingName, false);
        }
    }
}


void FeatureCalloutController::writeCalloutSetting(const QString& settingName, const bool& val)
{
    const Services::Setting* setting = NULL;
    Services::SettingsManager::instance()->getSettingByName(settingName, &setting);
    if (setting != NULL)
        Services::writeSetting(*setting, val, Services::Session::SessionService::currentSession());
    else
        ORIGIN_ASSERT_MESSAGE(0, "FeatureCalloutController::writeCalloutSetting failed.");
}


UIToolkit::OriginCallout* FeatureCalloutController::showCallout(const QString& settingName, const QString& blueTitle, const QString& title, const QString& message)
{
    UIToolkit::OriginLabel* label = new UIToolkit::OriginLabel();
    label->setLabelType(UIToolkit::OriginLabel::DialogDescription);
    label->setText(message);
    label->setWordWrap(true);
    return showCallout(settingName, blueTitle, title, label);
}


UIToolkit::OriginCallout* FeatureCalloutController::showCallout(const QString& settingName, const QString& blueTitle, const QString& title, QWidget* content)
{
    const CalloutSetting setting = calloutSettingBySettingName(settingName);
    return showCallout(settingName, UIToolkit::OriginCallout::calloutIconTemplate(setting.mBindedObjectName, blueTitle, title, content));
}


UIToolkit::OriginCallout* FeatureCalloutController::showCallout(const QString& settingName, const QString& blueTitle, const QString& title, QList<UIToolkit::OriginCalloutIconTemplateContentLine*> contentLines)
{
    const CalloutSetting setting = calloutSettingBySettingName(settingName);
    return showCallout(settingName, UIToolkit::OriginCallout::calloutIconTemplate(setting.mBindedObjectName, blueTitle, title, contentLines));
}


UIToolkit::OriginCallout* FeatureCalloutController::showCallout(const QString& settingName, UIToolkit::OriginCallout* callout)
{
    const CalloutSetting setting = calloutSettingBySettingName(settingName);
    if(mCalloutMap.value(setting.mBindedObjectName, NULL) == NULL)
    {
        mCalloutMap.insert(setting.mBindedObjectName, callout);
        ORIGIN_VERIFY_CONNECT(callout, SIGNAL(finished(int)), this, SLOT(onCalloutDone(const int&)));
        ORIGIN_VERIFY_CONNECT(callout, SIGNAL(dyingFromParent()), this, SLOT(onCalloutDyingFromParent()));

        ORIGIN_LOG_ACTION << "Feature Callout: Showing - Creating binded to " << setting.mBindedObjectName;
    }
    else
    {
        ORIGIN_LOG_ACTION << "Feature Callout: Showing - Already has callout binded to " << setting.mBindedObjectName;
        callout->deleteLater();
    }

    // Do we have a valid callout now? So it.
    if(callout)
    {
        callout->show();
    }
    // No? Woah... what happened? Perhaps this callout was suppose to be removed? Was it added to the callout settings?
    else
    {
        ORIGIN_ASSERT(callout);
    }

    return callout;
}



UIToolkit::OriginCallout* FeatureCalloutController::callout(const QString& parentObjectName)
{
    return mCalloutMap.value(parentObjectName, NULL);
}


void FeatureCalloutController::onCalloutDone(const int& result)
{
    UIToolkit::OriginCallout* callout = dynamic_cast<UIToolkit::OriginCallout*>(sender());
    if(callout)
    {
        const CalloutSetting setting = calloutSettingByBindName(callout->bindWidgetName());
        ORIGIN_LOG_ACTION << "Feature Callout: " << setting.mSettingName << " was " << ((result == 0) ? "dismissed" : "completed");
        writeCalloutSetting(setting.mSettingName, true);
        GetTelemetryInterface()->Metric_FEATURE_CALLOUT_DISMISSED(setting.mSeriesName.toStdString().c_str(), setting.mBindedObjectName.toStdString().c_str(), result == 1);
        mCalloutMap.remove(setting.mBindedObjectName);
        callout->deleteLater();
    }
}


void FeatureCalloutController::onCalloutDyingFromParent()
{
    // Remove from map. Callout was already deleted with the parent.
    UIToolkit::OriginCallout* callout = dynamic_cast<UIToolkit::OriginCallout*>(sender());
    if(callout)
    {
        ORIGIN_LOG_ACTION << "Feature Callout: call was deleted by parent( " << callout->bindWidgetName() << " ). Removing from callout map.";
        mCalloutMap.remove(callout->bindWidgetName());
    }
}

}
}