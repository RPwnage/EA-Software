;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var forceStringIds = function()
{
    tr("ebisu_client_language_zh_cn"),
    tr("ebisu_client_language_zh_tw");
    tr("ebisu_client_language_cs_cz");
    tr("ebisu_client_language_da_dk");
    tr("ebisu_client_language_nl_nl");    
    tr("ebisu_client_language_en_gb");
    tr("ebisu_client_language_en_us");
    tr("ebisu_client_language_fi_fi");
    tr("ebisu_client_language_fr_fr");
    tr("ebisu_client_language_de_de");    
    tr("ebisu_client_language_el_gr");
    tr("ebisu_client_language_hu_hu");    
    tr("ebisu_client_language_it_it");
    tr("ebisu_client_language_ja_jp");
    tr("ebisu_client_language_ko_kr");
    tr("ebisu_client_language_no_no");    
    tr("ebisu_client_language_pl_pl");
    tr("ebisu_client_language_pt_br");
    tr("ebisu_client_language_pt_pt");
    tr("ebisu_client_language_ru_ru");
    tr("ebisu_client_language_es_mx");
    tr("ebisu_client_language_es_es");
    tr("ebisu_client_language_sv_se");
    tr("ebisu_client_language_th_th");
    tr("ebisu_client_language_tr_tr");
}

var GeneralView = function()
{
    this.init();
}

var tooltipTop = function(obj)
{
    var siblingOffset = obj.siblings("a").css("bottom").replace('px','') / 3;
    var borderMargin = 34;
    return (obj.height() + borderMargin + siblingOffset) * -1;
}

GeneralView.prototype.init = function()
{
    $(window).blur(function()
    {
        $("#gen-ddClientLang").blur();
        $("#gen-ddDefaultTab").blur();
    });
    
    // Set localized text
    $("#gen-lblClientLang").text(tr('ebisu_client_lang_select_label').replace("%1", tr("application_name")));
    $("#gen-lblBetaParticipation").text(tr('ebisu_client_beta_title').replace("%1", tr("application_name")));
	$("#gen-chkOriginAutoUpdateText").text(tr('ebisu_client_settings_autoupdate').replace("%1", tr("application_name")));
    $("#gen-lblBetaText").text(tr('ebisu_client_beta_tooltip').replace("%1", tr("application_name")));
    $("#gen-lblCloudStorageText").text(tr('ebisu_client_cloud_text').replace(/%1/g, tr("application_name")));
    $('#gen-chkCloudstorageTooltip > span').text(tr('ebisu_client_must_be_online_to_change_setting'));
    $('#gen-chkCloudstorageTooltip > span').css('top', tooltipTop($('#gen-chkCloudstorageTooltip > span')));
    this.onConnectionStateChange();
    onlineStatus.onlineStateChanged.connect(this.onConnectionStateChange);

	if(Origin.views.currentPlatform() == "PC")
    {
        $("#gen-chkRunClientOnBootText").text(tr('ebisu_client_settings_autostart').replace("%1", tr("application_name")));
        $("#page_title").text(tr('ebisu_client_settings'));
    }
	else
    {
        $("#gen-chkRunClientOnBootText").text(tr('ebisu_client_settings_autostart_mac').replace("%1", tr("application_name")));
        $("#page_title").text(tr('ebisu_client_preferences'));
    }
    
    // We are not in an OIG context
    if(window.helper && window.helper.context() == 0)
    {
        // Listen for a change in our current top level entitlements.
        entitlementManager.topLevelEntitlements.forEach(this.listenForEntitlementChange);
        // Listen for titles to be added - then listen for the added title to change.
        entitlementManager.added.connect(this.listenForEntitlementChange);
        // We only update the state of the banner if our page is visible. Update when we become visible again.
        originPageVisibility.visibilityChanged.connect($.proxy(this, 'checkForEntitlementsActivity'));
        // Check to see if any of our entitlements have activity that should disable the change language combobox.
        this.checkForEntitlementsActivity();
    }
    // Hide the tooltips if we are in OIG
    else
    {
        this.setBannerState('ACTIVE_OIG');
    }
    
    this.setupLanguageList();
    this.setupDefaultTab();
    
    // Start Origin on platform boot
    $("#gen-chkRunClientOnBoot").on('change', function(evt)
    {
        clientSettings.writeSetting("RunOnSystemStart", this.checked);
    });
    
    // Chat show time stamp
    $("#gen-chkShowTimestamps").on('change', function(evt)
    {
        clientSettings.writeSetting("ShowTimeStamps", this.checked);
    });
    
    // Chat save conversation history
    $("#gen-chkSaveConversationHistory").on('change', function(evt)
    {
        clientSettings.writeSetting("SaveConversationHistory", this.checked);
    });
    
    // Game auto-patch
    $("#gen-chkGamesAutopatch").on('change', function(evt)
    {
        clientSettings.writeSetting("AutoPatch", this.checked);
        // save timestamp of when "AutoPatch" is modified to preserve it or overwrite depending on when a new install occurs (ORPLAT-1034)
        clientSettings.writeSetting("AutoPatchTimestamp", clientSettings.getTimestamp());   
    });
    
    // Game auto-update
    $("#gen-chkOriginAutoUpdate").on('change', function(evt)
    {
        clientSettings.writeSetting("AutoUpdate", this.checked);
    });
    
    $("#gen-chkIgnoreUpdates").on('change', function(evt)
    {
        clientSettings.writeSetting("IgnoreNonMandatoryGameUpdates", this.checked);
    });
	
    // Origin beta participation
    $("#gen-chkBeta").on('change', function(evt)
    {
        clientSettings.writeSetting("BetaOptIn", this.checked);
    });

    // Cloud Storage
    $("#gen-chkCloudstorage").on('click', function(evt)
    {
        if(clientSettings.areServerUserSettingsLoaded === false)
        {
            evt.preventDefault();
        }
        clientSettings.writeSetting("cloud_saves_enabled", this.checked);
    });
    
    // If the user is under age...
    if(originUser.commerceAllowed == false)
    {
        $('#gen-Chat').hide();
        $('#gen-ChatHr').hide();
    }
    
    clientSettings.updateSettings.connect($.proxy(this, 'onUpdateSettings'));
    this.onUpdateSettings();
}

GeneralView.prototype.onConnectionStateChange = function()
{
    if(onlineStatus.onlineState)
    {
        $('#gen-chkCloudstorageTooltip').hide();
        $('#gen-chkCloudstorage').prop('disabled', false);
    }
    else
    {
        $('#gen-chkCloudstorageTooltip').show();
        $('#gen-chkCloudstorage').prop('disabled', true);
    }
}

GeneralView.prototype.listenForEntitlementChange = function(ent)
{
    // We have to check all of the entitlements because we can be playing multiple
    // games at once. If one closes, we need to make sure another isn't open.
    ent.changed.connect($.proxy(GeneralView.prototype, 'checkForEntitlementsActivity'));
}

GeneralView.prototype.setupLanguageList = function()
{
	function compare(a,b)
    {
        if (a.string < b.string)  return -1;
        if (a.string > b.string)  return 1;
        return 0;
    }

    var Locales = [];
	var localeID;
    for(var i = 0, numLang = clientSettings.supportedlanguages.length; i < numLang; i++)
    {
        localeID = clientSettings.supportedlanguages[i];
        Locales.push({ id: localeID, string: tr("ebisu_client_language_" + localeID.toLowerCase()) });
    }
    Locales.sort(compare);

    var option = "";
    for(var i = 0, numLang = clientSettings.supportedlanguages.length; i < numLang; i++)
    {
        option = ('<option value="%0">%1</option>').replace("%0", Locales[i].id);
        option = option.replace("%1", Locales[i].string);
        $("#gen-ddClientLang").append(option);
    }
    
    // Set our client locale
    $("#gen-ddClientLang").on('change', function(evt)
    {
        clientSettings.writeSetting("Locale", this.value);
    });
}

GeneralView.prototype.setupDefaultTab = function()
{
   // Add tabs to the default tab drop down
   // Order must be: Origin Now, My Games, Store, Let Origin Decide
    var option = "";
    var isOriginNowEnabled = clientSettings.readSetting("ShowMyOrigin");
   
    // Origin Now
    // If the user isn't underage and Origin Now is enabled
    if (originUser.commerceAllowed && isOriginNowEnabled)
    {
        option = ('<option value="myorigin">%1</option>').replace("%1", tr("home_title"));
        $("#gen-ddDefaultTab").append(option);
    }
    // My Games
    option = ('<option value="mygames">%1</option>').replace("%1", tr("ebisu_client_my_games_lowercase"));
    $("#gen-ddDefaultTab").append(option);

    if(originUser.commerceAllowed)
    {
        // Store
        option = ('<option value="store">%1</option>').replace("%1", tr("ebisu_client_store_system_tray"));
        $("#gen-ddDefaultTab").append(option);
    
        // Let Origin Decide
        var string = tr('ebisu_client_let_origin_decide').replace("%1", tr("application_name"))
        option = ('<option value="decide">%1</option>').replace("%1", string);
        $("#gen-ddDefaultTab").append(option);
    }

    // If the default tab was Origin Now and Origin Now isn't enabled. Set the default to My Games.
    // Or if the user is underage. Set the default to My Games.
    if((clientSettings.readSetting("DefaultTab") == 'myorigin' && isOriginNowEnabled == false)
        || originUser.commerceAllowed == false)
    {
        clientSettings.writeSetting("DefaultTab", 'mygames');
    }
    
    // Origin default startup nav tab
    $("#gen-ddDefaultTab").on('change', function(evt)
    {
        clientSettings.writeSetting("DefaultTab", this.value);
    });
}

GeneralView.prototype.setBannerState = function(state)
{
    if(state == '')
    {
        $('#gen-bannerClientLang').hide();
        $("#gen-ddClientLang").parents(".origin-ux-drop-down-control").removeClass("disabled");
        $("#navbar > ul > li.current > span").css('background-image','url(settings/images/common/arrow.png)');
    }
	else
	{
        if(state == 'ACTIVE_OIG')
        {
            $("#gen-bannerClientLang").text(tr('ebisu_client_cant_change_lang_oig').replace("%1", tr("ebisu_client_igo_name")));
        }
        else if(state == 'ACTIVE_FLOW')
        {
            $("#gen-bannerClientLang").text(tr('ebisu_client_settings_language_restriction'));
        }
        else if(state == 'ACTIVE_PLAY')
        {
            $("#gen-bannerClientLang").text(tr('ebisu_client_settings_language_restriction_play').replace("%1", tr("application_name")));
        }

        $('#gen-bannerClientLang').show();
        $("#navbar > ul > li.current > span").css('background-image','url(settings/images/common/arrow-blue.png)');
        $("#gen-ddClientLang").parents(".origin-ux-drop-down-control").addClass("disabled");
    }
}

GeneralView.prototype.checkForEntitlementsActivity = function()
{
    // Only do checks and make changes if the page is visible
    if(originPageVisibility.hidden == false)
    {
        var entitlementsActivity = function()
        {
            // Any entitlements downloading or installing?
            var isActive = false;
            isActive = entitlementManager.topLevelEntitlements.some(function(ent)
            {   
                return (ent.downloadOperation !== null || ent.installOperation !== null);
            });
            if(isActive) return 'ACTIVE_FLOW';
        
            // Any entitlements being played?
            isActive = entitlementManager.topLevelEntitlements.some(function(ent)
            {
                return ent.playing;
            });
            if(isActive) return 'ACTIVE_PLAY';
            return '';
        }
        this.setBannerState(entitlementsActivity());
    }
}

GeneralView.prototype.onUpdateSettings = function()
{
    $("#gen-chkRunClientOnBoot").prop("checked", clientSettings.readSetting("RunOnSystemStart"));
    $("#gen-chkShowTimestamps").prop("checked", clientSettings.readSetting("ShowTimeStamps"));
    $("#gen-chkSaveConversationHistory").prop("checked", clientSettings.readSetting("SaveConversationHistory"));
    $("#gen-chkGamesAutopatch").prop("checked", clientSettings.readSetting("AutoPatch"));
    $("#gen-chkOriginAutoUpdate").prop("checked", clientSettings.readSetting("AutoUpdate"));
    $("#gen-chkBeta").prop("checked", clientSettings.readSetting("BetaOptIn"));
    $("#gen-chkCloudstorage").prop("checked", clientSettings.readSetting("cloud_saves_enabled"));
    $("#gen-ddClientLang").prop("value", clientSettings.readSetting("Locale"));
    $("#gen-ddDefaultTab").prop("value", clientSettings.readSetting("DefaultTab"));
    $("#gen-chkIgnoreUpdates").prop("checked", clientSettings.readSetting("IgnoreNonMandatoryGameUpdates"));	
}

// Expose to the world
Origin.views.general = new GeneralView();

})(jQuery);
