/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to settings with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'modules/client/communication'
], function(clientobjectregistry,communication) {

    /**
     * Contains client settings communication methods
     * @module module:settings
     * @memberof module:Origin.module:client
     *
     */
    var clientSettingsWrapper = null,
        clientObjectName = 'OriginClientSettings',
        isEmbeddedBrowser = communication.isEmbeddedBrowser;

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientSettingsWrapper = clientObjectWrapper;
        if (clientSettingsWrapper.clientObject) {
            clientSettingsWrapper.connectClientSignalToJSSDKEvent('updateSettings', 'CLIENT_SETTINGS_UPDATESETTINGS');
            clientSettingsWrapper.connectClientSignalToJSSDKEvent('returnFromSettingsDialog', 'CLIENT_SETTINGS_RETURN_FROM_DIALOG');
            clientSettingsWrapper.connectClientSignalToJSSDKEvent('settingsError', 'CLIENT_SETTINGS_ERROR');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:settings */ {
        /**
         * writes a setting in the client
         * @param {string} settingName the name of the setting
         * @param {string} payload the payload to write for the setting
         * @static
         * @method
         */
        writeSetting: function(settingName, payload) {
            return clientSettingsWrapper.sendToOriginClient('writeSetting', arguments);
        },

        /**
         * reads a setting from the client
         * @param {string} settingName the name of the setting
         * @static
         * @method
         */
        readSetting: function(settingName) {
            return clientSettingsWrapper.sendToOriginClient('readSetting', arguments);
        },
        /**
         * returns the client supported languages
         * @returns {languageObject} returnValue the client supported languages
         * @static
         * @method
         */
        supportedLanguagesData: function() {
            return clientSettingsWrapper.propertyFromOriginClient('supportedLanguagesData');
        },
        /**
         * returns whether the Origin Developer Tool is available
         * @returns {languageObject} returnValue if odt is available
         * @static
         * @method
         */
        developerToolAvailable: function() {
            if (!isEmbeddedBrowser()) {
                return false;
            } else {
                return clientSettingsWrapper.propertyFromOriginClient('developerToolAvailable');                
            }
        },
        /**
         * [startLocalHostResponder description]
         * @return {promise} retName TBD
         */
        startLocalHostResponder: function() {
            return clientSettingsWrapper.sendToOriginClient('startLocalHostResponder');
        },
        /**
         * [stopLocalHostResponder description]
         * @return {promise} retName TBD
         */
        stopLocalHostResponder: function() {
            return clientSettingsWrapper.sendToOriginClient('stopLocalHostResponder');
        },
        /**
         * [startLocalHostResponderFromOptOut description]
         * @return {promise} retName TBD
         */
        startLocalHostResponderFromOptOut: function() {
            return clientSettingsWrapper.sendToOriginClient('startLocalHostResponderFromOptOut');
        },
        /**
         * returns the setting that was swapped when a hotkey conflict occurred
         * @returns {object} returnValue the setting that was swapped and hot key string
         * @static
         * @method
         */
        hotkeyConflictSwap: function() {
            return clientSettingsWrapper.sendToOriginClient('hotkeyConflictSwap', arguments);
        },
        /**
         * tell the client to set the hotkey input state to either true or false
         * @param {bool} hasFocus the focus state of the hotkey
         */
        hotkeyInputHasFocus: function(hasFocus) {
            return clientSettingsWrapper.sendToOriginClient('hotkeyInputHasFocus', arguments);
        },
        /**
         * tell the client to set the window pinning hotkey input state to either true or false
         * @param {bool} hasFocus the focus state of the window pinning hotkey
         */
        pinHotkeyInputHasFocus: function(hasFocus) {
            return clientSettingsWrapper.sendToOriginClient('pinHotkeyInputHasFocus', arguments);
        },
        /**
         * tell the client to set the Push-To-Talk hotkey input state to either true or false
         * @param {bool} hasFocus the focus state of the Push-To-Talk hotkey
         */
        pushToTalkHotkeyInputHasFocus: function(hasFocus) {
            return clientSettingsWrapper.sendToOriginClient('pushToTalkHotKeyInputHasFocus', arguments);
        },
        /**
         * tell the client to set the Push-To-Talk hotkey to a mouse button
         * 0 = left, 1 = middle, 2 = right, 3 = browser back, 4 = browser forward
         * @param {int} button the mouse button that was clicked
         */
        handlePushToTalkMouseInput: function(button) {
            return clientSettingsWrapper.sendToOriginClient('handlePushToTalkMouseInput', arguments);
        },
        /**
         * tell the client whether SPA is considered offline capable based on whether all the files listed in index.html have been loaded or not
         * @param {bool} capable can load SPA in offline mode
         */
        setSPAofflineCapable: function(capable) {
            return clientSettingsWrapper.sendToOriginClient('setSPAofflineCapable', arguments);
        }
    };
});
