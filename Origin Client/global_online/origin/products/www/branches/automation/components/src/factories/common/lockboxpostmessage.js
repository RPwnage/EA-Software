/**
 * @file LockboxPostMessage.js
 */
(function() {
    'use strict';

    var MAX_WAIT_TIME = 90000,
        REGISTER_INTERVAL_TIME = 50,
        EVENTS = {
            SIZE_CHANGE: 'sizeChange',
            READY: 'pageReady',
            SUCCESS: 'paymentSuccess',
            REDIRECT_PAYMENT_PROVIDER_PREPARE: 'redirectPrepare',
            REDIRECT_PAYMENT_PROVIDER_READY: 'redirectReady',
            REDIRECT_PAYMENT_PROVIDER_CANCEL: 'redirectCancel',
            OPEN_LINK: 'openLink',
            DOWNLOAD_ORIGIN: 'downloadOrigin',
            DOWNLOAD_GAME: 'downloadGame',
            BACK_TO_GAME: 'backToGame',
            ERROR: 'error',
            SEND_MARKETING_EVENT: 'sendMarketingEvent',
            SEND_TRANSACTION_EVENT: 'sendTransactionEvent',
            TEALIUM_READY_EVENT: 'tealiumReadyEvent',
            CONTINUE_SHOPPING: 'continueShopping',
            SET_CUSTOM_DIMENSION: 'setCustomDimension'
        };

    function LockboxPostMessageFactory(PostMessageSendFactory, PostMessageReceiveFactory, ComponentsConfigHelper, BeaconFactory, UrlHelper) {
        var registerObject = {
                'method': 'register',
                'gameId': ComponentsConfigHelper.getSetting('gameId'),
                'currentHost': UrlHelper.getWindowOrigin(),
                'eventList': [
                    EVENTS.SIZE_CHANGE,
                    EVENTS.READY,
                    EVENTS.SUCCESS,
                    EVENTS.REDIRECT_PAYMENT_PROVIDER_PREPARE,
                    EVENTS.REDIRECT_PAYMENT_PROVIDER_READY,
                    EVENTS.REDIRECT_PAYMENT_PROVIDER_CANCEL,
                    EVENTS.OPEN_LINK,
                    EVENTS.DOWNLOAD_ORIGIN,
                    EVENTS.DOWNLOAD_GAME,
                    EVENTS.BACK_TO_GAME,
                    EVENTS.ERROR,
                    EVENTS.CONTINUE_SHOPPING
                ],
                'platform': Origin.client.isEmbeddedBrowser() ? 'client' : 'web'
            };

        function createInstance(origin, iframeElement, timeout) {
            var sender,
                reciever;

            function receive (eventName, callback, payloadAttr) {
                reciever.subscribe(eventName, function(payload) {
                    if (payload.hasOwnProperty(payloadAttr)) {
                        callback.call(this, payload[payloadAttr]);
                    } else {
                        callback.call(this, payload);
                    }
                });
            }

            function subscribeToPaymentProviderRedirectPrepare(callback) {
                receive(EVENTS.REDIRECT_PAYMENT_PROVIDER_PREPARE, callback);
            }

            function subscribeToPaymentProviderRedirectReady(callback) {
                receive(EVENTS.REDIRECT_PAYMENT_PROVIDER_READY, callback, 'uri');
            }

            function subscribeToPaymentProviderRedirectCancel(callback) {
                receive(EVENTS.REDIRECT_PAYMENT_PROVIDER_CANCEL, callback);
            }

            function subscribeToOpenLink(callback){
                receive(EVENTS.OPEN_LINK, callback, 'uri');
            }

            function subscribeToSizeChange(callback){
                receive(EVENTS.SIZE_CHANGE, callback, 'height');
            }

            function subscribeToSuccess(callback){
                receive(EVENTS.SUCCESS, callback);
            }

            function subscribeToError(callback){
                receive(EVENTS.ERROR, callback, 'code');
            }

            function subscribeToDownloadOrigin(callback){
                receive(EVENTS.DOWNLOAD_ORIGIN, callback);
            }

            function subscribeToDownloadGame(callback){
                receive(EVENTS.DOWNLOAD_GAME, callback, 'offerId');
            }

            function subscribeToBackToGame(callback){
                receive(EVENTS.BACK_TO_GAME, callback);
            }

            function subscribeToMarketingEvent(callback) {
                receive(EVENTS.SEND_MARKETING_EVENT, callback);
            }

            function subscribeToTransactionEvent(callback) {
                receive(EVENTS.SEND_TRANSACTION_EVENT, callback);
            }

            function subscribeToTealiumReadyEvent(callback) {
                receive(EVENTS.TEALIUM_READY_EVENT, callback);
            }

            function subscribeToContinueShoppingEvent(callback) {
                receive(EVENTS.CONTINUE_SHOPPING, callback);
            }

            function subscribeToCustomDimension(callback) {
                receive(EVENTS.SET_CUSTOM_DIMENSION, callback);
            }            

            function handshake() {
                return new Promise(function(resolve, reject){
                    var timeoutHandle,
                        onReadyInterval;

                    function onReady() {
                        // clear timeout and interval as we have successfully completed the handshake
                        window.clearTimeout(timeoutHandle);
                        window.clearInterval(onReadyInterval);
                        resolve();
                    }

                    function storeClientInstalledState(data) {
                        registerObject.clientIsInstalled = data;
                    }

                    function register() {
                        // Set timeout for 90 seconds - if we haven't received the 'success' event by then, error out.
                        timeoutHandle = setTimeout(function() {
                                window.clearInterval(onReadyInterval);
                                reject('timeout');
                            }, timeout || MAX_WAIT_TIME);

                        // Continually ping lockbox with out register event until we either get a 'success' message or timeout.
                        onReadyInterval = setInterval(function() {
                                sender.sendMessage(registerObject);
                            }, REGISTER_INTERVAL_TIME);

                        // Resolve promise and remove timeout/interval when successfully registered with lockbox
                        reciever.subscribe('success', onReady);
                    }

                    if (!Origin.client.isEmbeddedBrowser()){
                        BeaconFactory.isInstalled()
                            .then(storeClientInstalledState)
                            .then(register);
                    } else {
                        storeClientInstalledState(true);
                        register();
                    }
                });
            }

            sender = PostMessageSendFactory.createSender(origin, iframeElement);
            reciever = PostMessageReceiveFactory.createReceiver(origin, 'method', 'payload');

            return {
                handshake: handshake,
                destroy: reciever.destroy,
                subscribeToPaymentProviderRedirectPrepare: subscribeToPaymentProviderRedirectPrepare,
                subscribeToPaymentProviderRedirectReady: subscribeToPaymentProviderRedirectReady,
                subscribeToPaymentProviderRedirectCancel: subscribeToPaymentProviderRedirectCancel,
                subscribeToOpenLink: subscribeToOpenLink,
                subscribeToSizeChange: subscribeToSizeChange,
                subscribeToSuccess: subscribeToSuccess,
                subscribeToDownloadOrigin: subscribeToDownloadOrigin,
                subscribeToDownloadGame: subscribeToDownloadGame,
                subscribeToBackToGame: subscribeToBackToGame,
                subscribeToError: subscribeToError,
                subscribeToMarketingEvent: subscribeToMarketingEvent,
                subscribeToTransactionEvent: subscribeToTransactionEvent,
                subscribeToTealiumReadyEvent: subscribeToTealiumReadyEvent,
                subscribeToContinueShoppingEvent: subscribeToContinueShoppingEvent,
                subscribeToCustomDimension: subscribeToCustomDimension,
                send: sender.sendMessage
            };
        }
        return {
            createInstance: createInstance
        };
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.CheckoutFactory

     * @description
     *
     * Checkout Helper
     */
    angular.module('origin-components')
        .factory('LockboxPostMessageFactory', LockboxPostMessageFactory);
}());
