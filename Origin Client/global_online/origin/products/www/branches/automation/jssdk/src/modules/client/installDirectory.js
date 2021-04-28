/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to install directory management with the C++ client
 */


define([
    'modules/client/clientobjectregistry'
], function(clientobjectregistry) {

    /**
     * Contains client settings communication methods
     * @module module:installDirectory
     * @memberof module:Origin.module:client
     *
     */
    var clientInstallDirectoryWrapper = null,
        clientObjectName = 'InstallDirectoryManager';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientInstallDirectoryWrapper = clientObjectWrapper;
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:installDirectory */ {

        /**
         * [chooseDownloadInPlaceLocation description]
         * @return {promise} retName TBD
         */
        chooseDownloadInPlaceLocation: function() {
            return clientInstallDirectoryWrapper.sendToOriginClient('chooseDownloadInPlaceLocation');
        },
        /**
         * [resetDownloadInPlaceLocation description]
         * @return {promise} retName TBD
         */
        resetDownloadInPlaceLocation: function() {
            return clientInstallDirectoryWrapper.sendToOriginClient('resetDownloadInPlaceLocation');
        },
        /**
         * [chooseInstallerCacheLocation description]
         * @return {promise} retName TBD
         */
        chooseInstallerCacheLocation: function() {
            return clientInstallDirectoryWrapper.sendToOriginClient('chooseInstallerCacheLocation');
        },
        /**
         * [resetInstallerCacheLocation description]
         * @return {promise} retName TBD
         */
        resetInstallerCacheLocation: function() {
            return clientInstallDirectoryWrapper.sendToOriginClient('resetInstallerCacheLocation');
        },
        /**
         * [browseInstallerCache description]
         * @return {promise} retName TBD
         */
        browseInstallerCache: function() {
            return clientInstallDirectoryWrapper.sendToOriginClient('browseInstallerCache');
        },
        /**
         * [deleteInstallers description]
         * @return {promise} retName TBD
         */
        deleteInstallers: function() {
            return clientInstallDirectoryWrapper.sendToOriginClient('deleteInstallers');
        }
    };
});