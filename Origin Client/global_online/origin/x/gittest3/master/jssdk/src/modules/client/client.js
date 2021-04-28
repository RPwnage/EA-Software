/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication with the C++ client
 */
define([
    'core/events',
    'modules/client/games',
    'modules/client/dialog'
], function (events, clientGames, clientDialogs) {
    //check for the client online status change
    if (typeof OriginOnlineStatus !== 'undefined') {
        OriginOnlineStatus.onlineStateChanged.connect(function(state) {
            events.fire(events.CLIENT_ONLINESTATECHANGED, state);
        });
    }
    /** @namespace
     * @memberof Origin
     * @alias client
     */
    return {
        games: clientGames,
        dialogs: clientDialogs,
        /**
         * returns true if Origin client is online
         * @returns {boolean} true/false
         */
        isOnline: function() {
            //if we are embedded then we check for the C++ object OriginOnlineStatus
            //if not we always return true because stand alone browser is always online
            if (typeof OriginOnlineStatus !== 'undefined') {
                return OriginOnlineStatus.onlineState;
            } else {
                return true;
            }
        }
    };
});
