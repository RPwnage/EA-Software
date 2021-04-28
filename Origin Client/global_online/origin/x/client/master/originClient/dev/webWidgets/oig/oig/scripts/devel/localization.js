window.originTranslator = {
    fallbackStrings: {
    "application_name": "Origin",
    "ebisu_client_close": "Close",
    "ebisu_client_cancel": "Cancel",
    "ebisu_client_cancel": "Cancel",
    "ebisu_client_broadcast_gameplay_uppercase" : "BROADCAST YOUR GAMEPLAY",
    "ebisu_client_beta" : "Beta",
    "ebisu_client_broadcast_gameplay_text_to" : "Your broadcast will be streamed live to %1",
    "ebisu_client_basic_setup_uppercase" : "BASIC SETUP",
    "ebisu_client_advanced_setup_uppercase" : "ADVANCED SETTINGS",
    "ebisu_client_broadcast_name" : "Broadcast Title",
    "ebisu_client_auto_broadcast" : "Automatically broadcast when starting any game on Origin",
    "ebisu_client_broadcasting_url_before_broadcasting" : "Your broadcast will be streamed live to %1",
    "ebisu_client_connected_twitch" : "Connected to Twitch",
    "ebisu_client_stop_broadcast" : "Stop Broadcast",
    "ebisu_client_start_broadcast" : "Start Broadcast",
    "ebisu_client_disconnect" : "Disconnect",
    "ebisu_client_connect" : "Connect",
    "ebisu_client_disconnected_twitch" : "Not connected to Twitch ",
    "ebisu_client_critical_warning_broadcast" : "You are now broadcasting your gameplay to the Internet. DO NOT broadcast unreleased games unless you have a death wish.",
    "ebisu_client_no_twitch_archiving" : "Currently, your videos are not being archived. <a href=\"settings-archive\">Please visit Twitch settings to enable archiving.<\a>",
    "ebisu_client_twitch_introduction" : "%1 has partnered with Twitch to provide an easy way to broadcast your gameplay to everyone",
    "ebisu_client_twitch_connected_to_twitch_as_user" : "Connected to Twitch as %1",
    "ebisu_client_twitch_broadcast_your_game" : "Broadcast Your Game",
    "ebisu_client_twitch_connect_to_twitch" : "Connect to Twitch",
    "ebisu_client_microphone_volume" : "Voice Volume",
    "ebisu_client_mute" : "Mute",
    "ebisu_client_game_volume" : "Game Volume",
    "ebisu_client_broadcast_resolution" : "Broadcast Resolution",
    "ebisu_client_broadcast_framerate" : "Broadcast Framerate (fps)"
    },

    translate: function(string_id) {
        var fallbackStrings = originTranslator.fallbackStrings;

        if (string_id in fallbackStrings) {
            return fallbackStrings[string_id];
        }

        return string_id;
    }
}

window.tr = function(string_id) {
    return originTranslator.translate(string_id)
}
