/*jshint strict: false */
/*jshint unused: false */

define([
    'modules/voice/voiceBridge',
    'modules/voice/voiceWeb',
    'modules/client/client'
], function (voiceBridge, voiceWeb, client) {

    var voice = voiceWeb;
    //check and see if bridge exists
    if (client.isEmbeddedBrowser()) {
        console.log('using bridge voice');
        voice = voiceBridge;
    } else {
        console.log('using web voice');
    }

    voice.init();

    return voice;
});
