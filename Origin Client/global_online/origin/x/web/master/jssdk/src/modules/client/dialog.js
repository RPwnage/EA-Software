/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to dialogs with the C++ client
 */


define([
    'modules/client/clientobjectregistry',
    'core/errorhandler'
], function(clientobjectregistry, errorhandler) {

    /**
     * Contains client dialog communication methods
     * @module module:dialog
     * @memberof module:Origin.module:client
     *
     */


    var clientDialogWrapper = null,
        clientObjectName = 'OriginDialogs';

    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientDialogWrapper = clientObjectWrapper;
        if(clientDialogWrapper.clientObject){
            clientDialogWrapper.connectClientSignalToJSSDKEvent('dialogOpen', 'CLIENT_DIALOGOPEN');
            clientDialogWrapper.connectClientSignalToJSSDKEvent('dialogClosed', 'CLIENT_DIALOGCLOSED');
            clientDialogWrapper.connectClientSignalToJSSDKEvent('dialogChanged', 'CLIENT_DIALOGCHANGED');            
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName)
        .then(executeWhenClientObjectInitialized)
        .catch(errorhandler.logErrorMessage);


    return /** @lends module:Origin.module:client.module:dialog */ {


        /**
         * show Object
         * @typedef showObject
         * @type {object}
         * @property {string} context Where is this dialog coming from?
         */


        /**
         * show first dialog in queue
         * @param  {module:Origin.module:client.module:dialog~showObject} showObject object to be passed to client
         */
        show: function(retObj) {
            return clientDialogWrapper.sendToOriginClient('show', arguments);
        },

        /**
         * finished Object
         * @typedef finishedObject
         * @type {object}
         * @property {string} id a unique id for the dialog
         * @property {boolean} accepted was the dialog accepted
         * @property {object} content info C++ needs
         */

        /**
         * closes dialog in queue with match id. Passes info to client.
         * @param  {module:Origin.module:client.module:dialog~finishedObject} finishedObject object to be passed to client
         */
        finished: function(retObj) {
            return clientDialogWrapper.sendToOriginClient('finished', arguments);
        },

        /**
         * showingDialog Object
         * @typedef showingDialogObject
         * @type {object}
         * @property {string} id a unique id for the dialog
         */

        /**
         * Tells C++ queue that a dialog from the javascript is showing. Since dialogs
         * don't solely come from C++.
         * @param  {module:Origin.module:client.module:dialog~showingDialogObject} showingDialogObject object to be passed to client
         */
        showingDialog: function(retObj) {
            return clientDialogWrapper.sendToOriginClient('showingDialog', arguments);
        },

        /**
         * linkReact Object
         * @typedef linkReactObject
         * @type {object}
         * @property {string} href The link that the client needs to react to. Think of it as an id.
         */
        /**
         * Tells C++ that a link was just clicked inside of the dialog. The C++ needs to handle it.
         * @param  {module:Origin.module:client.module:dialog~linkReactObject} linkReactObject object to be passed to client
         */
        linkReact: function(retObj) {
            return clientDialogWrapper.sendToOriginClient('linkReact', arguments);
        }
    };
});