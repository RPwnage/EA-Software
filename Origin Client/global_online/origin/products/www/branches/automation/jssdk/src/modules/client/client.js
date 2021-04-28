/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication with the C++ client
 */
define([
    'core/events',
    'modules/client/communication',
    'modules/client/contentOperationQueue',
    'modules/client/games',
    'modules/client/dialog',
    'modules/client/social',
    'modules/client/socialui',
    'modules/client/onlinestatus',
    'modules/client/settings',
    'modules/client/user',
    'modules/client/desktopservices',
    'modules/client/voice',
    'modules/client/oig',
    'modules/client/installDirectory',
    'modules/client/info',
    'modules/client/popout',
    /** These modules don't need to be exposed but they need to be included somewhere so requirejs loads them.
        They simply wait for the client object to become available and hook up events if it is**/
    'modules/client/navigation',
    'modules/client/dirtybits'
], function(events, communication, clientContentOperationQueue, clientGames, clientDialogs, clientSocial, clientSocialUI, clientOnlineStatus, 
    clientSettings, clientUser, clientDesktopServices, clientVoice, clientOIG, clientInstallDirectory, clientInfo, clientPopout, clientNav /* navigation, dirtybits */) {

    return /** @lends module:Origin.module:client */ {
        games: clientGames,

        dialogs: clientDialogs,

        social: clientSocial,

        socialui: clientSocialUI,

        onlineStatus: clientOnlineStatus,

        settings: clientSettings,

        user: clientUser,

        desktopServices: clientDesktopServices,

        voice: clientVoice,

        oig: clientOIG,

        navigation: clientNav,

        contentOperationQueue: clientContentOperationQueue,

        installDirectory: clientInstallDirectory,

        info: clientInfo,

        popout: clientPopout,

        isEmbeddedBrowser: communication.isEmbeddedBrowser
    };
});