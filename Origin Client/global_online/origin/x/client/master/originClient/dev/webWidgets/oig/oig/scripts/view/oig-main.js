;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var OigMainView = function()
{
    $('#oig-sidenav-browser').on('click', function(e) {
        clientNavigation.showIGOBrowser();
    });
    $('#oig-sidenav-friends').on('click', function(e) {
        clientNavigation.showFriends();
    });
    $('#oig-sidenav-help').on('click', function(e) {
        clientNavigation.showOriginHelp();
    });
    $('#oig-sidenav-download').on('click', function(e) {
        clientNavigation.showDownloadProgressDialog();
    });
    $('#oig-sidenav-settings').on('click', function(e) {
        clientNavigation.showGeneralSettingsPage();
    });
    if(Origin.views.currentPlatform() === 'MAC')
    {
        $('#oig-sidenav-broadcast').hide();
    }
    $('#oig-sidenav-broadcast').on('click', function(e) {
        clientNavigation.showBroadcast();
    });
};

// Expose to the world
Origin.views.oigMain = new OigMainView();

})(jQuery);