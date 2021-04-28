;(function($){
"use strict";

if (!window.Origin) { window.Origin = {}; }
if (!Origin.views) { Origin.views = {}; }

var OigBraodcastConnectView = function()
{
    $('.broadcast-header .otktitle-2').append(' (' + tr('ebisu_client_beta') + ')');
    $('.oig-broadcast-header-message').text(tr('ebisu_client_twitch_introduction').replace("%1", tr('application_name')));
    $('.connect-twitch-cta').click(function() {
        broadcast.showLogin();
    });
};

// Expose to the world
Origin.views.oigBroadcastConnect = new OigBraodcastConnectView();

})(jQuery);