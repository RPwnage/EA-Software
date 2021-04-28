/*jshint unused: false */
/*jshint strict: false */
define([
    'modules/client/client'
], function(client) {
    //need to set all localStorage entries to be dirty to force retrieval of LMD
    function markAllLMDdirty() {
        //do this only if we're online
        var online = true;
        if (client.isEmbeddedBrowser()) {
            online = client.onlineStatus.isOnline();
        }
        if (online) {
            for (var i = 0; i < localStorage.length; i++) {
                if (localStorage.key(i).indexOf('lmd_') === 0) {
                    var lmdEntryStr = localStorage.getItem(localStorage.key(i));
                    var offerCacheInfo = JSON.parse(lmdEntryStr);
                    //mark it as dirty
                    offerCacheInfo.dirty = true;

                    var jsonS = JSON.stringify(offerCacheInfo);
                    localStorage.setItem(localStorage.key(i), jsonS);
                }
            }
        }
    }
    return {
        markAllLMDdirty: markAllLMDdirty
    };
});