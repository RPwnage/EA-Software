/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'modules/voice/voiceBridge',
    'modules/voice/voiceWeb'
], function (voiceBridge, voiceWeb) {

    //check and see if bridge exists
    if (typeof OriginVoice !== 'undefined') {
        console.log('using bridge voice');
        voice = voiceBridge;
    } else {
        console.log('using web voice');
        voice = voiceWeb;
    }

    return voice;
});
