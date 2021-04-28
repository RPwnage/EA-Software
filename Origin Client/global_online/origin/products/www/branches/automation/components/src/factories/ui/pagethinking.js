/**
 * @file factories/ui/pagethinking.js
 */
(function () {
    'use strict';

    function PageThinkingFactory() {

        var uiBlockRequests = 0;
        var communicator = new Origin.utils.Communicator();


        /**
         * Increments block requests and notifies listeners.
         * pagethinking  directive is listening to these changes.
         */
        function blockUI() {
            uiBlockRequests++;
            notifySubscribersIfNecessary();
        }

        /**
         * Decrements block requests and notifies listeners.
         * pagethinking  directive is listening to these changes.
         */
        function unblockUI() {
            if (uiBlockRequests > 0) {
                uiBlockRequests--;
                notifySubscribersIfNecessary();
            }
        }

        /**
         * Block and unblocks screen while function is running.
         * @param func
         */
        function blockUIForTask(func) {
            if (_.isFunction(func)) {
                blockUI();
                func();
                unblockUI();
            }
        }

        /**
        * Block and unblocks screen while promise is running.
        * @param {Promise} func
        * @returns {Promise} resolved object
        */
        function blockUIForPromise(promise) {
            blockUI();
            return promise.then(unblockUIPromise)
               .catch(unblockUIPromiseOnError);
        }

        /**
        * Convenience method to blockUI in a promise chain
        * @param passThroughObject object to resolve the promise with (for chainability)
        * @returns {Promise}
        */
        function blockUIPromise(passThroughObject) {
            blockUI();
            return Promise.resolve(passThroughObject);
        }

        /**
        * Convenience method to unblockUI in a promise chain
        * @param passThroughObject object to resolve the promise with (for chainability)
        * @returns {Promise}
        */
        function unblockUIPromise(passThroughObject) {
            unblockUI();
            return Promise.resolve(passThroughObject);
        }

        /**
        * Convenience method to blockUI in a promise chain
        * @param passThroughObject object to resolve the promise with (for chainability)
        * @returns {Promise}
        */
        function unblockUIPromiseOnError(passThroughObject) {
            unblockUI();
            return Promise.reject(passThroughObject);
        }

        /**
         * Determins whether UI is currently blocked.
         * @returns {boolean}
         */
        function isBlocked() {
            return uiBlockRequests > 0;
        }

        /**
         * Determins if subscribers should to be notified.
         */
        function notifySubscribersIfNecessary() {
            if (uiBlockRequests === 1) {
                notifySubscribers();
            } else if (uiBlockRequests === 0) {
                notifySubscribers();
            }
        }

        function notifySubscribers() {
            communicator.fire('uiBlock');
        }

        function subscribeToChanges(callback) {
            return communicator.on('uiBlock', callback).detach;
        }

        function unsubscribeFromChanges(callback) {
            communicator.off('uiBlock', callback);
        }


        return {
            isBlocked: isBlocked,
            blockUI: blockUI,
            blockUIForTask: blockUIForTask,
            blockUIForPromise: blockUIForPromise,
            blockUIPromise: blockUIPromise, //promise wrapped version of blockUI
            unblockUI: unblockUI,
            unblockUIPromise: unblockUIPromise,//promise wrapped version of unblockUI
            unblockUIPromiseOnError: unblockUIPromiseOnError,
            subscribeToChanges: subscribeToChanges,
            unsubscribeFromChanges: unsubscribeFromChanges
        };

    }


    /**
     * @ngdoc service
     * @name origin-components.factories.PageThinkingFactory

     * @description Blocks the UI when a long task is in progress. Relies on origin-pagethinking directive
     *
     * PageThinkingFactory
     */
    angular.module('origin-components')
        .factory('PageThinkingFactory', PageThinkingFactory);

}());
