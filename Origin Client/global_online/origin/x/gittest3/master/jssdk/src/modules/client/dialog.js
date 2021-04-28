/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to dialogs with the C++ client
 */

/** @namespace
 * @memberof Origin.client
 * @alias dialogs
 */
define([
    'core/events',
    'modules/client/utils'
], function (events, clientUtils) {
    var sendToOriginClient = clientUtils.sendToOriginClient;
    var listenForOriginClientMsg = clientUtils.listenForOriginClientMsg;
    var dialogObj = 'OriginDialogs';

    listenForOriginClientMsg(dialogObj, 'dialogOpen', function(status) {
        events.fire(events.CLIENT_DIALOGOPEN, status);
    });

    listenForOriginClientMsg(dialogObj, 'dialogClosed', function(id) {
        events.fire(events.CLIENT_DIALOGCLOSED, id);
    });
    
    listenForOriginClientMsg(dialogObj, 'dialogChanged', function(status) {
        events.fire(events.CLIENT_DIALOGCHANGED, status);
    });

    return {
        /**
         * show first dialog in queue
         * @param  {Object} info to pass to the origin client
         *   context: Where is this dialog coming from?
         * @memberof Origin.client.dialogs
         */
        show: function(retObj) {
            return sendToOriginClient(dialogObj, 'show', retObj);
        },
        /**
         * closes dialog in queue with match id. Passes info to client.
         * @param  {Object} productId the product id of the game
         *   id: 'unique id', // Unique id of dialog that needs to close
         *   accepted: true/false, // Was the dialog accepted
         *   content : {} // info C++ needs
         * @memberof Origin.client.dialogs
         */
        finished: function(retObj) {
            return sendToOriginClient(dialogObj, 'finished', retObj);
        },
        /**
         * Tells C++ queue that a dialog from the javascript is showing. Since dialogs
         * don't solely come from C++.
         * @param  {Object} productId the product id of the game
         *   id: 'unique id' // Unique id of dialog that needs to close
         * @memberof Origin.client.dialogs
         */
        showingDialog: function(retObj) {
            return sendToOriginClient(dialogObj, 'showingDialog', retObj);
        },
        /**
         * Tells C++ that a link was just clicked inside of the dialog. The C++ needs to handle it.
         * @param  {Object}
         *   href: The link that the client needs to react to. Think of it as an id.
         * @memberof Origin.client.dialogs
         */
        linkReact: function(retObj) {
            return sendToOriginClient(dialogObj, 'linkReact', retObj);
        }
    };
});