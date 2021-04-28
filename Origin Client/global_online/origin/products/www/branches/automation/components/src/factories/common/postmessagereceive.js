/**
 * Factory for receiving post message events from an iframe
 * @file common/postmessagereceive.js
 */
(function() {
    'use strict';

    function PostMessageReceiveFactory(ComponentsLogFactory) {

        /*
         * Factory instance creation function. Creates and returns a communicator with which to subscribe to iframe postMessages.
         * @param {string} originUrl The origin of the iframe you are listening to
         * @param {string} eventNameKey The key for the event name of the iframe post message event you are listening to. Ie: event.data.method or event.data.name
         * @param {string} eventPayloadKey The key for the event payload of the post message event you are listening to. Ie: event.data.payload or event.data.data
         */
        function createReceiver(originUrl, eventNameKey, eventPayloadKey) {

            var communicator = new Origin.utils.Communicator(),
                listening = false,
                messageHandler;

            function isAcceptedOrigin(event) {
                // For Chrome, the origin property is in the event.originalEvent object.
                var origin = event.origin || event.originalEvent.origin;
                return origin === originUrl;
            }

            /**
             * Returns a function that filters the the postmessage communicator to only fire if the origin of the event comes from the configured origin
             * @param {object} communicator
             * @return {function} Function that filters postmessage callback to accepted origins only
             */
            function handleMsg(communicator) {
                return function (event) {
                    if (isAcceptedOrigin(event)) {
                        var data = event.data;

                        if (!angular.isObject(data)){
                            try {
                                data = JSON.parse(event.data);
                            } catch (e) {
                                ComponentsLogFactory.error('Iframe postmessage data is un-parseable. Not able to fire event. Data: ' + data.toString(), e);
                                return;
                            }
                        }

                        communicator.fire(data[eventNameKey], data[eventPayloadKey]);
                    }
                };
            }

            /**
             * Start listening to the window for postmessages
             */
            function addListener(communicator) {
                messageHandler = handleMsg(communicator);
                window.addEventListener('message', messageHandler, false);
            }

            /*
             * Listen to a postmessage event
             * @param {string} eventName The name of the event you want to listen for
             * @param {function} callback The callback funtion to be executed on this event
             */
            function subscribe(eventName, callback) {
                if(!listening){
                    addListener(communicator);
                    listening = true;
                }

                communicator.on(eventName, callback);
            }

            /*
             * Stop listening to a postmessage event
             * @param {string} eventName The name of the event you were listening to
             * @param {function} callback The callback funtion you want to unsubscribe
             */
            function unsubscribe(eventName, callback) {
                communicator.off(eventName, callback);
            }

            /**
             * Remove postMessage event listener from the window
             */
            function destroy() {
                if(angular.isDefined(messageHandler)) {
                    window.removeEventListener('message', messageHandler);
                }
            }

            return {
                subscribe: subscribe,
                unsubscribe: unsubscribe,
                destroy: destroy
            };
        }

        return {
            createReceiver: createReceiver
        };
    }

    angular.module('origin-components')
        .factory('PostMessageReceiveFactory', PostMessageReceiveFactory);
})();


