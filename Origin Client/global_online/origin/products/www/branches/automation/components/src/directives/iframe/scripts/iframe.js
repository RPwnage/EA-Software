/**
 * @file iframe/scripts/iframe.js
 */

(function(){
    'use strict';

    /**
     * CheckoutTypeEnumeration list of allowed types
     * @enum {string}
     */
    /* jshint ignore:start */
    var CheckoutTypeEnumeration = {
        "default": "default",
        "subs": "subs",
        "gift" : "gift",
        "default-oig": "default-oig",
        "topup": "topup",
        "topup-oig": "topup-oig"
    };
    /* jshint ignore:end */

    var MODAL_DIALOG_CLASS = '.otkmodal-dialog';

    function originIframe(ComponentsConfigFactory, LockboxPostMessageFactory, $sce, AnimateFactory, PageThinkingFactory, CheckoutFactory, ScreenFactory, AppCommFactory, OriginCheckoutTypesConstant) {
        function originIframeLink(scope, element) {
            var iframe = element.find('iframe'),
                communicator,
                oldHeight;

            function resizeIframe(newHeight) {
                if (scope.type === OriginCheckoutTypesConstant.DEFAULT_OIG || scope.type === OriginCheckoutTypesConstant.TOPUP_OIG || scope.type === OriginCheckoutTypesConstant.CHAINED) {
                    // OIG dialogs are not responsive to size change events so ignore them.
                    return;
                }

                var dialogHeight = element.parents(MODAL_DIALOG_CLASS).height();

                // Handle case where newHeight is not defined - use previously cached height
                newHeight = newHeight || oldHeight;
                oldHeight = newHeight;

                if (!ScreenFactory.isSmall() && dialogHeight) {
                    newHeight = Math.min(newHeight, dialogHeight);
                }

                iframe.height(newHeight);
            }

            function handleError(error) {
                PageThinkingFactory.unblockUI();
                //stop the performance timer
                Origin.performance.endTime('COMPONENTSPERF:checkoutHandshake');

                AppCommFactory.events.fire('origin-iframe:error', error);
                CheckoutFactory.handleError(error);
            }

            /*
             * To be called when the iframe is ready to be shown.
             */
            function onReady() {
                PageThinkingFactory.unblockUI();
                AppCommFactory.events.fire('origin-iframe:ready');
                resizeIframe(angular.element('body').outerHeight() - 160); // 160 is the size of the padding on the dialog

                // Tell lockbox that we have displayed the iframe
                communicator.send({method: 'display'});
                Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTSPERF:checkoutHandshake');
            }

            /**
             * Called when tealium is ready and loaded.
             */
            function onTealiumReady() {
                // Send the userId to link telemetry data from iframe w/ SPA
                communicator.send({
                    method: 'telemetryInfo',
                    payload: {
                        /* jshint camelcase: false */
                        utag: window.utag_data
                        /* jshint camelcase: true */
                    }
                });
            }

            /*
             * Subscribe to relevant postmessage events.
             */
            function subscribeToEvents(communicator) {
                // subscribe to relevant postMessages
                communicator.subscribeToSizeChange(resizeIframe);
                communicator.subscribeToSuccess(CheckoutFactory.onPaymentSuccess);
                communicator.subscribeToPaymentProviderRedirectPrepare(CheckoutFactory.paymentProviderRedirectPrepare);
                communicator.subscribeToPaymentProviderRedirectReady(CheckoutFactory.paymentProviderRedirectReady.bind(null, scope.type));
                communicator.subscribeToPaymentProviderRedirectCancel(CheckoutFactory.paymentProviderRedirectCancel);
                communicator.subscribeToOpenLink(CheckoutFactory.openLink);
                communicator.subscribeToDownloadOrigin(CheckoutFactory.downloadOrigin);
                communicator.subscribeToDownloadGame(CheckoutFactory.downloadGame);
                communicator.subscribeToBackToGame(CheckoutFactory.backToGame);
                communicator.subscribeToError(handleError);
                communicator.subscribeToMarketingEvent(CheckoutFactory.sendMarketingEvent);
                communicator.subscribeToTransactionEvent(CheckoutFactory.sendTransactionEvent);
                communicator.subscribeToTransactionEvent(_.partial(CheckoutFactory.updateWishlist, scope.type));
                communicator.subscribeToTealiumReadyEvent(onTealiumReady);
                communicator.subscribeToContinueShoppingEvent(CheckoutFactory.close);
                communicator.subscribeToCustomDimension(CheckoutFactory.setCustomDimension);
            }

            function onIframeLoad() {
                Origin.performance.endTimeMeasureAndSendTelemetry('COMPONENTSPERF:loadLockBox');
                Origin.performance.beginTime('COMPONENTSPERF:checkoutHandshake');

                communicator = LockboxPostMessageFactory.createInstance(scope.originUrl, iframe);
                subscribeToEvents(communicator);
                communicator.handshake()
                    .then(onReady)
                    .catch(handleError);
            }

            function onIframeDestroy() {
                // close communicator to allow close events to be processed
                setTimeout(function() {
                    if (angular.isDefined(communicator)) {
                        communicator.destroy();
                    }
                }, 1000);
            }

            /*
             * Instantiates the lockbox communicator. Blocks the UI until handshake completes then subscribes to postmessage events.
             */
            function init() {
                PageThinkingFactory.blockUI();

                // Need to wait for iframe to load properly before continuing otherwise contentWindow will be null and we wont be able to attach a postMessage listener
                // on the content window.
                iframe.one('load', onIframeLoad);
                iframe.one('$destroy', onIframeDestroy);

                // load iframe
                scope.url = $sce.trustAsResourceUrl(scope.sourceUrl);

                AnimateFactory.addResizeEventHandler(scope, resizeIframe, 100);
            }

            init();
        }

        return {
            restrict: 'E',
            scope: {
                sourceUrl: '@',
                originUrl: '@',
                type: '@',
                loadingTitle: '@',
                loadingMessage: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('iframe/views/iframe.html'),
            link: originIframeLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originIframe
     * @restrict E
     * @element ANY
     * @scope
     * @param {Url} source-url Url for the iframe source.
     * @param {Url} origin-url Url for the iframe origin.
     * @param {CheckoutTypeEnumeration} type checkout type (subs or default)
     * @param {LocalizedString} loading-title The title that should be displayed on the loading page for third party payment providers ex. 'Your Order Is Being Processed'
     * @param {LocalizedString} loading-message The message that should be displayed on the loading page for third party payment providers ex. 'To ensure your order is not interrupted..'
     * @description Creates an iframe that is integrated with Lockbox
     *
     * @example
     * <origin-iframe source-url="#" origin-url="#">
     * </origin-iframe>
     */
    angular.module('origin-components')
        .directive('originIframe', originIframe);
}());
