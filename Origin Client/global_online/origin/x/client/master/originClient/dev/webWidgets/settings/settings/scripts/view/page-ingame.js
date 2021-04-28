;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var tooltipTop = function(obj)
{
    var siblingOffset = obj.siblings("a").css("bottom").replace('px','') / 3;
    var borderMargin = 34;
    return (obj.height() + borderMargin + siblingOffset) * -1;
}

var InGameView = function()
{
    if(Origin.views.currentPlatform() == "PC")
    {
        $("#page_title").text(tr('ebisu_client_settings'));
    }
	else
    {
        $("#page_title").text(tr('ebisu_client_preferences'));
    }
    // Set text with %1 in them and tooltips
    $("#oig-lblOIGEnabled").text(tr('ebisu_client_enable_in_game_dashboard').replace("%1", tr("ebisu_client_igo_name")));
    
    $("#oig-lblHotKeys").text(tr('ebisu_client_dashboard_hotkeys').replace("%1", tr("ebisu_client_igo_name")));
    
    $('#oig-tooltipAllGamesEnabled').text(tr('ebisu_client_in_game_all_tooltip').replace(/%1/g, tr("ebisu_client_igo_name")));
    $('#oig-tooltipAllGamesEnabled').css('top', tooltipTop($('#oig-tooltipAllGamesEnabled')));
    
	$("#oig-leHotKeys").keyup = this.setHotKeyInputFocus(true);
    $("#oig-leHotKeys").focusin = this.setHotKeyInputFocus(true);
    $("#oig-leHotKeys").focusout = this.setHotKeyInputFocus(false);
    $("#oig-lePinKeys").keyup = this.setPinHotKeyInputFocus(true);
    $("#oig-lePinKeys").focusin = this.setPinHotKeyInputFocus(true);
    $("#oig-lePinKeys").focusout = this.setPinHotKeyInputFocus(false);
    
    if(window.helper && window.helper.context() == 0)
    {
        // Listen for a change in our current top level entitlements.
        entitlementManager.topLevelEntitlements.forEach(this.listenForEntitlementChange);
        // Listen for titles to be added - then listen for the added title to change.
        entitlementManager.added.connect(this.listenForEntitlementChange);
        // We only update the state of the banner if our page is visible. Update when we become visible again.
        originPageVisibility.visibilityChanged.connect($.proxy(this, 'onOigEnabledChanged'));
    }
	
	document.onclick = function()
	{
		// We need to deselect the OIG Hotkey if the user presses the "Restore Default"
		// button. However, focusout isn't working. So easy solution (might not be the best)
		// is so compare what the current object in focus is.
		var clickElementID = window.event.srcElement.id;
		if(clickElementID === "oig-leHotKeys")
		{
			Origin.views.inGame.setHotKeyInputFocus(true);
		}
		else if(clickElementID === "oig-btnRestore")
		{
			Origin.views.inGame.setHotKeyInputFocus(true);
			clientSettings.restoreDefaultHotkey("HotKey", "HotKeyString");
		}
        else if(clickElementID === "oig-lePinKeys")
		{
			Origin.views.inGame.setPinHotKeyInputFocus(true);
		}
		else if(clickElementID === "oig-btnPinRestore")
		{
			Origin.views.inGame.setPinHotKeyInputFocus(true);
			clientSettings.restoreDefaultHotkey("PinHotKey", "PinHotKeyString");
		}
		else
		{
			Origin.views.inGame.setHotKeyInputFocus(false);
            Origin.views.inGame.setPinHotKeyInputFocus(false);
		}
	}
    
    clientSettings.updateSettings.connect($.proxy(this, 'onUpdateSettings'));
    this.onUpdateSettings();
    
    // OIG enabled
    $("#oig-chkOIGEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("EnableIgo", this.checked);
        Origin.views.inGame.onOigEnabledChanged();
    });
    
    // Supported Games
    $("#oig-rbSupportedGamesEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("in_game_all_games", !this.checked);
    });
    
    // All games
    $("#oig-rbAllGamesEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("in_game_all_games", this.checked);
    });
    
    // OIG show welcome message
    $("#oig-chkOIGWelcomeEnabled").on('change', function(evt)
    {
        clientSettings.writeSetting("ShowIGONux", !this.checked);
    });
};

InGameView.prototype.listenForEntitlementChange = function(ent)
{
    // We have to check all of the entitlements because we can be playing multiple
    // games at once. If one closes, we need to make sure another isn't open.
    ent.changed.connect($.proxy(InGameView.prototype, 'onOigEnabledChanged'));
}

InGameView.prototype.isAnEntitlementActive = function()
{
    var isActive = false;
    // Only do checks and make changes if the page is visible
    if(originPageVisibility.hidden == false)
    {
        // Any entitlements being playing?
        isActive = entitlementManager.topLevelEntitlements.some(function(ent)
        {   
            return ent.playing === true;
        });
    }
    console.log("isAnEntitlementActive "+isActive);
    return isActive;
}

InGameView.prototype.onUpdateSettings = function()
{
    this.onOigEnabledChanged();
    
    $("#oig-rbSupportedGamesEnabled").prop("checked", !clientSettings.readSetting("in_game_all_games"));
    
    $("#oig-rbAllGamesEnabled").prop("checked", clientSettings.readSetting("in_game_all_games"));
    
    $("#oig-chkOIGWelcomeEnabled").prop("checked", !clientSettings.readSetting("ShowIGONux"));
    
    $('#oig-leHotKeys').val(clientSettings.readSetting("HotKeyString"));
    
    $('#oig-lePinKeys').val(clientSettings.readSetting("PinHotKeyString"));
}

InGameView.prototype.onOigEnabledChanged = function()
{
    var oigEnabled = clientSettings.readSetting("EnableIgo");
    $("#oig-chkOIGEnabled").prop("checked", oigEnabled);
    
    // Disable "Enable Origin In Game" and sub objects if:
    // - A game is currently being played.
    $("#oig-chkOIGEnabled").prop("disabled", this.isAnEntitlementActive());
    $("#oig-rbSupportedGamesEnabled").prop('disabled', $("#oig-chkOIGEnabled").is(":disabled") || !oigEnabled);
    $("#oig-rbAllGamesEnabled").prop('disabled', $("#oig-chkOIGEnabled").is(":disabled") || !oigEnabled);
    
    $("#oig-leHotKeys").prop('disabled', !oigEnabled);
    $("#oig-lePinKeys").prop('disabled', !oigEnabled);
    $("#oig-btnRestore").prop('disabled', !oigEnabled);
    $("#oig-chkOIGWelcomeEnabled").prop('disabled', !oigEnabled);
};

InGameView.prototype.setHotKeyInputFocus = function(hasFocus)
{
	if(hasFocus)
	{
		$("#oig-leHotKeys").select();
        if(window.helper)
        {
            window.helper.pinHotkeyInputHasFocus(false);
            window.helper.hotkeyInputHasFocus(true);
        }
	}
	else
	{
		if (window.getSelection)
        {
            window.getSelection().empty();
        }
        if(window.helper) window.helper.hotkeyInputHasFocus(false);
	}
}

InGameView.prototype.setPinHotKeyInputFocus = function(hasFocus)
{
	if(hasFocus)
	{
		$("#oig-lePinKeys").select();
        if(window.helper)
        {
            window.helper.hotkeyInputHasFocus(false);
            window.helper.pinHotkeyInputHasFocus(true);
        }
	}
	else
	{
		if (window.getSelection)
        {
            window.getSelection().empty();
        }
        if(window.helper) window.helper.pinHotkeyInputHasFocus(false);
	}
}

// Expose to the world
Origin.views.inGame = new InGameView();

})(jQuery);
