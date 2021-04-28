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
    'modules/client/onlinestatus',
    'modules/client/settings',
    'modules/client/user',
    'modules/client/desktopservices',
    'modules/client/voice',
    'modules/client/oig',
    'modules/client/installDirectory'
], function(events, communication, clientContentOperationQueue, clientGames, clientDialogs, clientSocial, clientOnlineStatus, clientSettings, clientUser, clientDesktopServices, clientVoice, clientOIG, clientInstallDirectory) {

    return /** @lends module:Origin.module:client */ {
        games: clientGames,

        dialogs: clientDialogs,

        social: clientSocial,

        onlineStatus: clientOnlineStatus,

        settings: clientSettings,

        user: clientUser,

        desktopServices: clientDesktopServices,

        voice: clientVoice,

        oig: clientOIG,

        contentOperationQueue: clientContentOperationQueue,

        installDirectory: clientInstallDirectory,

        isEmbeddedBrowser: communication.isEmbeddedBrowser
    };
});