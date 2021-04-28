;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var NavBarView = function()
{
    this.init();
	this.globalFunctionality();
}

NavBarView.prototype.init = function()
{
    var inOIGContext = window.helper && window.helper.context() != 0;
        
    var navbar = "<ul>\
        <li data-page='general' onclick=\"Origin.views.navbar.generalClicked()\" class='current'><a href=\"#\">" + tr("ebisu_client_general") + "</a><span></span></li>\
        <li data-page='notifications' onclick=\"Origin.views.navbar.notificationsClicked()\" class='current'><a href=\"#\">" + tr("ebisu_client_notifications") + "</a><span></span></li>";

    // If voice is supported (don't show to underage users)
    if(originVoice && originVoice.supported && originUser.commerceAllowed)
        navbar += "<li data-page='voice' onclick=\"Origin.views.navbar.voiceClicked()\" class='current'><a href=\"#\">" + tr("ebisu_client_voice_chat") + "</a><span></span></li>";

    navbar += "<li data-page='ingame' onclick=\"Origin.views.navbar.originInGameClicked()\" class='current'><a href=\"#\">" + tr("ebisu_client_igo_name") + "</a><span></span></li>";

    // If we aren't in OIG
    if(inOIGContext == false)
        navbar += "<li data-page='advanced' onclick=\"Origin.views.navbar.advancedClicked()\" class='current'><a href=\"#\">" + tr("ebisu_client_advanced") + "</a><span></span></li>";

    navbar += "</ul>";
    $("#navbar").append(navbar);
    this.currentNavOption();
}

// This is kind of bad - I should make a new view for global settings.
// However, since the nav bar has to be on every page I will just set it here.
// If this gets too big - we should pull this out.
NavBarView.prototype.globalFunctionality = function()
{
	// Add our platform attribute for the benefit of CSS
    $(document.body).addClass(Origin.views.currentPlatform());
}

NavBarView.prototype.currentNavOption = function()
{
    var page = $('.container').attr('data-page');
    var container = $('#navbar').find('ul li');
    container.removeClass('current');
    container.each(function(){
        ($(this).attr('data-page') == page)? $(this).addClass('current'): '';
    });

    // stop voice connection
    if (Origin.views.currentPlatform() === 'PC')
    {
        if (page == 'voice')
        {
            if( originVoice && originVoice.supported)
                window.setTimeout(originVoice.setInVoiceSettings(true), 0);
        }
        else
        {
            if( originVoice && originVoice.supported)
                window.setTimeout(originVoice.setInVoiceSettings(false), 0);
        }
    }
}

NavBarView.prototype.generalClicked = function()
{
    window.location.href = './general.html';
}

NavBarView.prototype.notificationsClicked = function()
{
    window.location.href = './notifications.html';
}

NavBarView.prototype.originInGameClicked = function()
{
    window.location.href = './ingame.html';
}

NavBarView.prototype.voiceClicked = function()
{
    window.location.href = './voice.html';
}

NavBarView.prototype.advancedClicked = function()
{
    window.location.href = './advanced.html';
}

// Expose to the world
Origin.views.navbar = new NavBarView();

})(jQuery);
