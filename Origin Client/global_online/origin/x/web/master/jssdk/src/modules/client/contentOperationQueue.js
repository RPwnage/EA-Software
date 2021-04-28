/*jshint unused: false */
/*jshint undef:false */
/*jshint strict: false */

/**
 * this module handles all the communication related to origin operation (download/install/update/repair) queue
 */


define([
    'modules/client/clientobjectregistry',
    'modules/client/communication'
], function(clientobjectregistry, communication) {
    var clientContentOperationQueueWrapper = null,
        clientObjectName = 'OriginContentOperationQueueController';

    /**
     * Contains client content operation queue communication methods
     * @module module:origincontentoperationqueue
     * @memberof module:Origin.module:client
     *
     */
    function executeWhenClientObjectInitialized(clientObjectWrapper) {
        clientContentOperationQueueWrapper = clientObjectWrapper;
        if (clientContentOperationQueueWrapper.clientObject) {
            clientContentOperationQueueWrapper.connectClientSignalToJSSDKEvent('enqueued', 'CLIENT_OPERATIONQUEUE_ENQUEUED');
            clientContentOperationQueueWrapper.connectClientSignalToJSSDKEvent('removed', 'CLIENT_OPERATIONQUEUE_REMOVED');
            clientContentOperationQueueWrapper.connectClientSignalToJSSDKEvent('addedToComplete', 'CLIENT_OPERATIONQUEUE_ADDEDTOCOMPLETE');
            clientContentOperationQueueWrapper.connectClientSignalToJSSDKEvent('completeListCleared', 'CLIENT_OPERATIONQUEUE_COMPLETELISTCLEARED');
            clientContentOperationQueueWrapper.connectClientSignalToJSSDKEvent('headBusy', 'CLIENT_OPERATIONQUEUE_HEADBUSY');
            clientContentOperationQueueWrapper.connectClientSignalToJSSDKEvent('headChanged', 'CLIENT_OPERATIONQUEUE_HEADCHANGED');
        }
    }

    clientobjectregistry.registerClientObject(clientObjectName).then(executeWhenClientObjectInitialized);

    return /** @lends module:Origin.module:client.module:origincontentoperationqueue */ {
        /**
         * Removes entitlement with passed in offer id from queue.
         * @param {offerId} offerId of the entitlement we are removing
         * @static
         * @method
         */
        remove: function(offerId) {
            clientContentOperationQueueWrapper.sendToOriginClient('remove', arguments);
        },

        /**
         * Pushes entitlement with passed in offer id to the top of the queue.
         * @param {offerId} offerId of the entitlement we are pushing to the top of the queue.
         * @static
         * @method
         */
        pushToTop: function(offerId) {
            clientContentOperationQueueWrapper.sendToOriginClient('pushToTop', arguments);
        },

        /**
         * Returns index of offer id in the queue. Returns -1 if it isn't there.
         * @param {offerId} offerId of the entitlement.
         * @returns {int} index/position of entitlement in queue
         * @static
         * @method
         */
        index: function(offerId) {
            return clientContentOperationQueueWrapper.sendToOriginClient('index', arguments);
        },

        /**
         * Returns true/false if entitlement is in the queue.
         * @param {offerId} offerId of the entitlement.
         * @returns {Boolean} if entitlement is in queue.
         * @static
         * @method
         */
        isInQueue: function(offerId) {
            return clientContentOperationQueueWrapper.sendToOriginClient('isInQueue', arguments);
        },

        /**
         * Returns true/false if entitlement is in the queue or in completed list.
         * @param {offerId} offerId of the entitlement.
         * @returns {Boolean} if entitlement is in queue or in the completed list.
         * @static
         * @method
         */
        isInQueueOrCompleted: function(offerId) {
            return clientContentOperationQueueWrapper.sendToOriginClient('isInQueueOrCompleted', arguments);
        },

        /**
         * Returns true/false if entitlement the entitlement can move to the front of the queue.
         * @param {offerId} offerId of the entitlement.
         * @returns {Boolean} if entitlement if entitlement can move to the front of the queue.
         * @static
         * @method
         */
        isQueueSkippingEnabled: function(offerId) {
            return clientContentOperationQueueWrapper.sendToOriginClient('queueSkippingEnabled', arguments);
        },

        /**
         * Returns true/false if the head entitlement of the queue is busy.
         * @returns {Boolean} true/false if the head of the queue is busy.
         * @static
         * @method
         */
        isHeadBusy: function() {
            return clientContentOperationQueueWrapper.sendToOriginClient('isHeadBusy');
        },

        /**
         * Clears the completed list.
         * @static
         * @method
         */
        clearCompleteList: function() {
            clientContentOperationQueueWrapper.sendToOriginClient('clearCompleteList');
        },

        /**
         * Returns true/false if entitlement's parent is in the list.
         * @param {offerId} offerId of the child entitlement.
         * @returns {Boolean} Returns true/false if entitlement's parent is in the list.
         * @static
         * @method
         */
        isParentInQueue: function() {
            return clientContentOperationQueueWrapper.sendToOriginClient('isParentInQueue', arguments);
        },

        /**
         * Returns offer id/product id of the head item in queue
         * @returns {Object} Returns offer id/product id of the head item in queue
         * @static
         * @method
         */
        headOfferId: function() {
            return clientContentOperationQueueWrapper.propertyFromOriginClient('headOfferId');
        },

        /**
         * Returns list of offer ids/prodct ids in the queue
         * @returns {StringList} Returns list of offer ids/prodct ids in the queue
         * @static
         * @method
         */
        entitlementsQueuedOfferIdList: function() {
            return clientContentOperationQueueWrapper.propertyFromOriginClient('entitlementsQueuedOfferIdList');
        },

        /**
         * Returns list of offer ids/prodct ids in the completed list
         * @returns {StringList} Returns list of offer ids/prodct ids in the complete list
         * @static
         * @method
         */
        entitlementsCompletedOfferIdList: function() {
            return clientContentOperationQueueWrapper.propertyFromOriginClient('entitlementsCompletedOfferIdList');
        }
    };
});