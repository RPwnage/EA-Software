/**
 * Factory for sending post messages to an iframe
 * @file common/postmessagesend.js
 */
(function() {
    'use strict';

    function PostMessageSendFactory() {

        /*
         * Intstantiates a sender object that will send messages to a given iframe and its origin
         * @param {object} $iframe A jQuery object of the iFrame
         * @param {string} iframeOrign The origin of the iframe
         * @return {object} Interface for sending a message
         */
        function createSender (iframeOrigin, $iframe) {
            var contentWindow = $iframe[0].contentWindow;

            /*
             * Sends a message to the instantiated iframe
             * @param {object} payload The data that you want to send to the iframe
             */
            function sendMessage (payload) {
                contentWindow.postMessage(JSON.stringify(payload), iframeOrigin);
            }

            return {
                sendMessage: sendMessage
            };
        }

        return {
            createSender: createSender
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.PostMessageSendFactory
     * @requires
     * @description
     *
     * PostMessageSendFactory
     */
    angular.module('origin-components')
        .factory('PostMessageSendFactory', PostMessageSendFactory);

})();