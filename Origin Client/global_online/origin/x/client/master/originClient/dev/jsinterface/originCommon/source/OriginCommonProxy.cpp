/////////////////////////////////////////////////////////////////////////////
// OriginCommonProxy.cpp
//
// Copyright (c) 2013, Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////

#include "OriginCommonProxy.h"
#include "originmsgbox.h"
#include "originwindow.h"
#include "engine/config/ConfigFile.h"
#include "engine/content/Entitlement.h"
#include "engine/content/ContentController.h"
#include "services/platform/PlatformService.h"
#include "services/crypto/SimpleEncryption.h"
#include "services/qt/QtUtil.h"
#include "TelemetryAPIDLL.h"

using namespace Origin::Services;

namespace Origin
{
namespace Client
{
namespace JsInterface
{
OriginCommonProxy::OriginCommonProxy()
{
    // Load the current set of overrides into memory so we have something to compare against.
    Engine::Config::ConfigFile::reloadCurrentConfigFile();
}

OriginCommonProxy::~OriginCommonProxy()
{
}

QString OriginCommonProxy::readOverride(QString sectionName, QString overrideName)
{
    return Engine::Config::ConfigFile::modifiedConfigFile().getOverride(sectionName, overrideName);
}

QString OriginCommonProxy::readOverride(QString sectionName, QString overrideName, QString id)
{
    return Engine::Config::ConfigFile::modifiedConfigFile().getOverride(sectionName, overrideName, id);
}
    
void OriginCommonProxy::writeOverride(QString sectionName, QString overrideName, QString value)
{
    Engine::Config::ConfigFile::modifiedConfigFile().updateOrDefaultOverride(sectionName, overrideName, value);
}

void OriginCommonProxy::writeOverride(QString sectionName, QString overrideName, QString id, QString value)
{
    QString offerId, contentId;

    Engine::Content::EntitlementRef entitlement = Engine::Content::ContentController::currentUserContentController()->entitlementById(id);

    if (!entitlement.isNull() && entitlement->contentConfiguration())
    {
        offerId = entitlement->contentConfiguration()->productId();
        contentId = entitlement->contentConfiguration()->contentId();
    }
    else
    {
        // A contentId will never be passed for unowned games, since EC2 no longer supports query by contentId.
        // We can safely assume it's an offerId!
        offerId = id;
        contentId = "";
    }

    Engine::Config::ConfigFile::modifiedConfigFile().updateOrDefaultOverride(sectionName, overrideName, offerId, value);
    
    // We want to wind up with a single override, specified by offer ID.
    // Remove the contentId-based override if necessary. 
    // NOG contentIds are the same as their productIds so do not remove if the two are equal.
    if (!contentId.isEmpty() && contentId.compare(offerId, Qt::CaseInsensitive) != 0)
    {
        Engine::Config::ConfigFile::modifiedConfigFile().removeOverride(sectionName, overrideName, contentId);
    }
}

bool OriginCommonProxy::reloadConfig()
{
    return Engine::Config::ConfigFile::modifiedConfigFile().loadConfigFile(Services::PlatformService::eacoreIniFilename());
}

bool OriginCommonProxy::applyChanges()
{
    bool restartRequired = false;

    GetTelemetryInterface()->Metric_ODT_APPLY_CHANGES();
    
    if(Engine::Config::ConfigFile::modifiedConfigFile().saveConfigFile(Services::PlatformService::eacoreIniFilename()))
    {
        // Detect any changes between the original file and the modified file.
        const Engine::Config::ChangedSettingsSet& changedSettings =
            Engine::Config::ConfigFile::detectOverrideChanges();

        // EBIBUGS-26907
        // Now that we've calculated any changes, replace our "current" config file
        // with the one we've just modified.  This lets us compare any future changes
        // against the overrides that are currently active.
        Engine::Config::ConfigFile::applyModifiedConfigFile();

        Engine::Config::ChangedSettingsSet::const_iterator iter;
        for (iter = changedSettings.begin(); iter != changedSettings.end(); ++iter)
        {
            const QString& sectionName = iter->first;
            const QString& settingName = iter->second;
            const QString& settingValue = Engine::Config::ConfigFile::modifiedConfigFile().getOverride(iter->first, iter->second);

            GetTelemetryInterface()->Metric_ODT_OVERRIDE_MODIFIED(sectionName.toUtf8().data(),
                settingName.toUtf8().data(), settingValue.toUtf8().data());
        }
        
        bool productOverrideChanged = false;
        bool generalOverrideChanged = false;
        for (iter = changedSettings.begin(); iter != changedSettings.end(); ++iter)
        {
            const QString& settingName = iter->second;
            if(Services::SettingsManager::instance()->isProductOverrideSetting(settingName))
            {
                productOverrideChanged = true;
            }
            else
            {
                generalOverrideChanged = true;
            }
        }

        // If any of the general settings (any settings that aren't product override settings) 
        // were changed at any point in this session, then the client needs to restart.
        restartRequired |= generalOverrideChanged;
    
        if(productOverrideChanged)
        {
            // Apply any product override setting changes
            Engine::Content::ContentController::currentUserContentController()->refreshUserGameLibrary(Engine::Content::CacheOnly);
        }
    }

    return restartRequired;
}
 
void OriginCommonProxy::alert(const QString &message, const QString &title, const QString &icon)
{
    UIToolkit::OriginMsgBox::IconType iconType;
    QString upperIcon = icon.toUpper();

    if ("ERROR" == upperIcon)
    {
        iconType = UIToolkit::OriginMsgBox::Error;
    }
    else if ("INFO" == upperIcon)
    {
        iconType = UIToolkit::OriginMsgBox::Info;
    }
    else if ("NOTICE" == upperIcon)
    {
        iconType = UIToolkit::OriginMsgBox::Notice;
    }
    else if ("SUCCESS" == upperIcon)
    {
        iconType = UIToolkit::OriginMsgBox::Success;
    }
    else
    {
        iconType = UIToolkit::OriginMsgBox::NoIcon;
    }

    UIToolkit::OriginWindow::alert(iconType, title, message, tr("ebisu_client_notranslate_close"));
}

void OriginCommonProxy::launchExternalBrowser(QString urlstring)
{
    QUrl url(QUrl::fromEncoded(urlstring.toUtf8()));
    PlatformService::asyncOpenUrl(url);
    GetTelemetryInterface()->Metric_ODT_NAVIGATE_URL(Services::EncodeUrlForTelemetry(url).data());
}

void OriginCommonProxy::viewSDDDocumentation()
{
    launchExternalBrowser(OBFUSCATE_STRING("https://developer.origin.com/documentation/display/OriginPublishing/Origin+Shift+Distributor"));
}

}
}
}
