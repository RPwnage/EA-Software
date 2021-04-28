/**
 * @file iframe/scripts/modal.js
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

    function originIframeModal(ComponentsConfigFactory, AppCommFactory, ScreenFactory) {
        function originIframeModalLink(scope, element) {
            function handleError() {
                scope.close();
            }

            /*
             * To be called when the iframe is ready to be shown.
             * Sets isReady to display the iframe. Adds a class to disable scroll on the body.
             */
            function onReady() {
                // Show iframe
                scope.isReady = true;
                scope.$digest();

                ScreenFactory.disableBodyScrolling();
            }

            function onDestroy() {
                AppCommFactory.events.off('origin-iframe:ready', onReady);
                AppCommFactory.events.off('origin-iframe:error', handleError);
            }

            /*
             * Instantiates the lockbox communicator.
             */
            function init() {
                AppCommFactory.events.on('checkout:close', scope.close);

                AppCommFactory.events.on('origin-iframe:ready', onReady);
                AppCommFactory.events.on('origin-iframe:error', handleError);

                scope.$on('$destroy', onDestroy);
            }

            /*
             * Removes the iframe from the DOM and destroys scope.
             */
            scope.close = function() {
                AppCommFactory.events.off('checkout:close', scope.close);

                ScreenFactory.enableBodyScrolling();

                // Remove element from DOM
                element.remove();

                //destroy scope
                scope.$destroy();
            };

            init();
        }

        return {
            restrict: 'E',
            scope: {
                sourceUrl: '@',
                originUrl: '@',
                type: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('iframe/views/modal.html'),
            link: originIframeModalLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originIframeModal
     * @restrict E
     * @element ANY
     * @scope
     * @param {Url} source-url Url for the iframe source.
     * @param {Url} origin-url Url for the iframe origin.
     * @param {CheckoutTypeEnumeration} type checkout type (subs or default)
     * @description Creates an iframe that is loaded up as a modal window. Integrates with Lockbox
     *
     * @example
     * <origin-iframe-modal source-url="#" origin-url="#">
     * </origin-iframe-modal>
     */
    angular.module('origin-components')
        .directive('originIframeModal', originIframeModal);
}());
