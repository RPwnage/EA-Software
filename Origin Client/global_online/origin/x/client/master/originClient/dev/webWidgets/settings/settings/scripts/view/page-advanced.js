;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var tooltipTop = function(obj)
{
    var siblingOffset = obj.css("bottom").replace('px','') / 3;
    var borderMargin = 5;
    return (obj.height() + borderMargin + siblingOffset);
}

var AdvancedView = function()
{
    $(window).blur(function()
    {
        $("#adv-ddCrashReports").blur();
    });

    if(Origin.views.currentPlatform() === "PC")
    {
        $("#page_title").text(tr('ebisu_client_settings'));
    }
	else
    {
        $("#page_title").text(tr('ebisu_client_preferences'));
    }
    // Set text with %1 in them and tooltips
    $("#adv-lblOriginCrashTitle").text(tr('ebisu_client_crash_reporting_title').replace("%1", tr("application_name")));
    $("#adv-lblOriginExpTitle").text(tr('ebisu_client_experience_reporting_title').replace("%1", tr("application_name")));
    $("#adv-lblOIGLoggingEnabled").text(tr('ebisu_client_enable_igo_log').replace("%1", tr("ebisu_client_igo_name")));
	$("#adv-lblOIGLoggingWarning").text(tr('ebisu_client_igo_logging_warning').replace("%1", tr("ebisu_client_igo_name")));
    $("#adv-lblLocalHostEnabled").text(tr('ebisu_client_enable_web_integration').replace("%1", tr("application_name")));
	$("#adv-lblLocalHostOptOut").text(tr('ebisu_client_web_integration_description').replace(/%1/g, tr("application_name")));
	$("#adv-lblLocalHostTitle").text(tr('ebisu_client_web_integration_title').replace("%1", tr("application_name")));
    $("#adv-lblQuietModeDesc").text(tr('ebisu_client_quiet_mode_description').replace("%1", tr("application_name")));
    $("#adv-lblFeaturePromoDesc").text(tr('ebisu_client_disable_promomanager_description').replace(/%1/g, tr("application_name")));
    $("#adv-lblAutoExitDesc").text(tr('ebisu_client_automatic_exit_description').replace("%1", tr("application_name")));
    $("#adv-lblHidePromos").text(tr('ebisu_client_disable_promomanager').replace("%1", tr("application_name")));
    $("#adv-lblAutoClose").text(tr('ebisu_client_exit_origin_after_closing_game').replace("%1", tr("application_name")));
    
    var faqLink = "<a class='originlabel_Description blue' onclick=Origin.views.advanced.loadFAQUrl()>" + tr('ebisu_client_faq') + "</a>";
    var originExpText = tr('ebisu_client_experience_reporting_text').replace(/%1/g, tr("application_name"));
    $("#adv-lblOriginExpText").html(originExpText.replace("%2", faqLink));
    
    clientSettings.updateSettings.connect($.proxy(this, 'onUpdateSettings'));
    this.onUpdateSettings();
    
    // Listen for a change in our current top level entitlements.
    entitlementManager.topLevelEntitlements.forEach(this.listenForEntitlementChange);
    // Listen for titles to be added - then listen for the added title to change.
    entitlementManager.added.connect(this.listenForEntitlementChange);
    // We only update the state of the banner if our page is visible. Update when we become visible again.
    originPageVisibility.visibilityChanged.connect($.proxy(this, 'checkForEntitlementsActivity'));
    // Check to see if any of our entitlements have activity that should disable the change language combobox.
    this.checkForEntitlementsActivity();
    
    // - Game download and install folder - 
    $("#adv-btnGamePathBrowse").on('click', function(evt)
    {
        window.setTimeout(function () {
            installDirectoryManager.chooseDownloadInPlaceLocation();
        }, 0);
    });
    
    // Reset to default
    $("#adv-btnGamePathDefault").on('click', function(evt)
    {
        installDirectoryManager.resetDownloadInPlaceLocation();
    });    

    
    // - Game Installers -
    $("#adv-btnInstallPathBrowse").on('click', function(evt)
    {
        window.setTimeout(function () {
           installDirectoryManager.chooseInstallerCacheLocation();
        }, 0);
    });
    
    $("#adv-btnInstallPathDefault").on('click', function(evt)
    {
        installDirectoryManager.resetInstallerCacheLocation();
    });
    
    // Keep game installers
    $("#adv-chkKeepInstallers").on('change', function(evt)
    {
		clientSettings.writeSetting("KeepInstallers", this.checked);
    });

    // Local Host Opt Out (now only for enabling - OFM-11618)
    $("#adv-chkLocalHostEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("LocalHostEnabled", this.checked);
        clientSettings.writeSetting("LocalHostResponderEnabled", this.checked);
        if(this.checked)
        {
            clientSettings.startLocalHost();

            $("#adv-chkLocalHostResponderEnabled").prop("checked", true);
        }
		else
        {
            $("#adv-chkLocalHostResponderEnabled").prop("checked", false);
        }
    });

    // Local Host Responder Opt Out
    $("#adv-chkLocalHostResponderEnabled").on('change', function (evt) {
        clientSettings.writeSetting("LocalHostResponderEnabled", this.checked);
        if (this.checked) {
            clientSettings.startLocalHostResponder();
        }
        else {
            clientSettings.stopLocalHostResponder();
        }
    });
    
    // Safe Mode
    $("#adv-chkSafeModeEnabled").on('change', function(evt)
    {
		clientSettings.writeSetting("EnableDownloaderSafeMode", this.checked);
    });
    
    $("#adv-btnOpenInstallersLocation").on('click', function(evt)
    {
        window.setTimeout(function () {
            installDirectoryManager.browseInstallerCache();
        }, 0);
    });
    
    $("#adv-btnDeleteInstallerFiles").on('click', function(evt)
    {
        installDirectoryManager.deleteInstallers();
    });
    
    $("#adv-chkHidePromos").on('change', function(evt)
    {
        clientSettings.writeSetting("DisablePromoManager", this.checked);
    });
    
    $("#adv-chkAutoClose").on('click', function(evt)
    {
        clientSettings.writeSetting("ShutDownOriginOnGameFinished", this.checked);
    });
    
    // Send hardware info
    $("#adv-chkSendHardwareEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("TelemetryHWSurveyOptOut", this.checked === false);
    });
    
    // Send usage info
    $("#adv-chkSendUsageEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("telemetry_enabled", this.checked);
    });

    // Crash report
    this.setupCrashReportList();
    
    // OIG Logging
    $("#adv-chkOIGLoggingEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("EnableInGameLogging", this.checked);
    });
    
    // If the user is under age...
    if(originUser.commerceAllowed === false)
    {
        $('#adv-featuredToday').hide();		
		$('#adv-featuredTodayHr').hide();
		$('#adv-OIGLogging').hide();		
		$('#adv-OIGLoggingHr').hide();
        $('#adv-CrashReport').hide();
        $('#adv-CrashReportHr').hide();
        $('#adv-Telemetry').hide();
        $('#adv-TelemetryHr').hide();
    }

	// Hide Origin Helper (aka LocalHost enable) if already enabled (OFM-11618)
	if (clientSettings.localHostInitialSetting() === true)
	{
		$('#adv-localHost').hide();
		$('#adv-localHostHr').hide();
	}
};

AdvancedView.prototype.listenForEntitlementChange = function(ent)
{
    // We have to check all of the entitlements because we can be playing multiple
    // games at once. If one closes, we need to make sure another isn't open.
    ent.changed.connect($.proxy(AdvancedView.prototype, 'checkForEntitlementsActivity'));
}

AdvancedView.prototype.setEntitlementState = function(state)
{
    console.log(state);
    if(state === '')
    {
        $('#adv-bannerGamePath').hide();
        $("#adv-btnGamePathBrowse").prop('disabled', false);
        $("#adv-btnGamePathDefault").prop('disabled', false);
        $("#adv-btnDeleteInstallerFiles").prop('disabled', false);
        $("#adv-btnInstallPathBrowse").prop('disabled', false);
        $("#adv-btnInstallPathDefault").prop('disabled', false);
    }
	else if(state === 'ACTIVE_FLOW')
	{
        $("#adv-btnGamePathBrowse").prop('disabled', true);
        $("#adv-btnGamePathDefault").prop('disabled', true);
        $("#adv-btnDeleteInstallerFiles").prop('disabled', true);
        $("#adv-btnInstallPathBrowse").prop('disabled', true);
        $("#adv-btnInstallPathDefault").prop('disabled', true);
        if($('#adv-bannerGamePath').is(":visible") === false)
        {
            $('#adv-bannerGamePath').show();
        }
    }
}

AdvancedView.prototype.checkForEntitlementsActivity = function()
{
    // Only do checks and make changes if the page is visible
    if(originPageVisibility.hidden === false)
    {
// This code is disabled because we no longer want to prevent the user from changing download directories when flows are active.
// Leaving this code in place in case we need this functionality for something else later
//        var entitlementsActivity = function()
//        {
//            // Any entitlements downloading or installing?
//            var isActive = false;
//            isActive = entitlementManager.topLevelEntitlements.some(function(ent)
//            {   
//                return (ent.downloadOperation !== null || ent.installOperation !== null);
//            });
//            if(isActive) return 'ACTIVE_FLOW';
//            return '';
//        }
//        this.setEntitlementState(entitlementsActivity());
        this.setEntitlementState('');
    }
}

AdvancedView.prototype.setupCrashReportList = function()
{
    // Order must be: Auto send, Never send, Ask before sending
    var option = "";
   
    $("#adv-ddCrashReports").append('<option>%1</option>'.replace("%1", tr("ebisu_client_automatically_send")));
    $("#adv-ddCrashReports").append('<option>%1</option>'.replace("%1", tr("ebisu_client_never_send")));
    $("#adv-ddCrashReports").append('<option>%1</option>'.replace("%1", tr("ebisu_client_ask_before_sending")));

    $("#adv-ddCrashReports").on('change', function(evt)
    {
        clientSettings.writeSetting("TelemetryCrashDataOptOut", this.selectedIndex);
    });
    $("#adv-ddCrashReports").prop('selectedIndex', clientSettings.readSetting("TelemetryCrashDataOptOut"));
}

AdvancedView.prototype.onUpdateSettings = function()
{
    $('#adv-leGamePath').val(clientSettings.readSetting("DownloadInPlaceDir"));
    $('#adv-leInstallPath').val(clientSettings.readSetting("CacheDir"));
    $("#adv-chkOIGLoggingEnabled").prop("checked", clientSettings.readSetting("EnableInGameLogging"));
    $("#adv-chkSendHardwareEnabled").prop("checked", !clientSettings.readSetting("TelemetryHWSurveyOptOut"));
    $("#adv-chkSendUsageEnabled").prop("checked", clientSettings.readSetting("telemetry_enabled"));
	$("#adv-chkKeepInstallers").prop("checked", clientSettings.readSetting("KeepInstallers"));
	$("#adv-chkLocalHostEnabled").prop("checked", clientSettings.readSetting("LocalHostEnabled"));
	$("#adv-chkLocalHostResponderEnabled").prop("checked", clientSettings.readSetting("LocalHostResponderEnabled"));
    $("#adv-chkSafeModeEnabled").prop("checked", clientSettings.readSetting("EnableDownloaderSafeMode"));
    $("#adv-chkHidePromos").prop("checked", clientSettings.readSetting("DisablePromoManager"));
    $("#adv-chkAutoClose").prop("checked", clientSettings.readSetting("ShutDownOriginOnGameFinished"));
}

AdvancedView.prototype.loadFAQUrl = function()
{
    var localizedFAQURL = clientSettings.readSetting("OriginFaqURL");
    var locale = clientSettings.readSetting("Locale");
	locale = locale.replace("_", "-");
    if(locale === "en-GB")
        locale = "en-uk";
    localizedFAQURL = localizedFAQURL.replace("%1", locale);
    clientNavigation.launchExternalBrowser(localizedFAQURL);
}



// Expose to the world
Origin.views.advanced = new AdvancedView();

})(jQuery);
