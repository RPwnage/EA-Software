/**
 * @file iframe/scripts/genericmodal.js
 */

(function(){
    'use strict';

    /* jshint ignore:start */
    var GenericModalScrollbarsEnumeration = {
        "yes": "yes",
        "no": "no"
    };
    /* jshint ignore:end */

    function originIframeGenericmodal(ComponentsConfigFactory, $sce, PageThinkingFactory, ScreenFactory) {
        function originIframeGenericmodalLink(scope, element) {
            var iframe = element.find('iframe'),
                modalRoot = element.find('.origin-iframemodal .otkmodal-dialog');

            function onIframeLoad() {
                PageThinkingFactory.unblockUI();
                modalRoot.height(scope.height);
                modalRoot.width(scope.width);
                iframe.width(scope.width);
                iframe.height(scope.height);

                // Show iframe
                scope.isReady = true;
                scope.$digest();

                ScreenFactory.disableBodyScrolling();
            }

            /*
             * Instantiates the lockbox communicator. Blocks the UI until handshake completes then subscribes to postmessage events.
             */
            function init() {
                PageThinkingFactory.blockUI();

                // Need to wait for iframe to load properly before continuing otherwise contentWindow will be null and we wont be able to attach a postMessage listener
                // on the content window.
                iframe.one('load', onIframeLoad);

                // load iframe
                scope.url = $sce.trustAsResourceUrl(scope.sourceUrl);
            }

            /*
             * Removes the iframe from the DOM and destroys scope.
             */
            scope.close = function() {
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
                height: '@',
                width: '@',
                scrollbars: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('iframe/views/genericmodal.html'),
            link: originIframeGenericmodalLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originIframeGenericmodal
     * @restrict E
     * @element ANY
     * @scope
     * @param {Url} source-url Url for the iframe source.
     * @param {integer} height height of the modal
     * @param {integer} width width of the modal
     * @param {GenericModalScrollbarsEnumeration} scrollbars whether to include scrollbars ("yes" or "no")
     * @description Creates an iframe that is loaded up as a modal window. Integrates with Lockbox
     *
     * @example
     * <origin-iframe-genericmodal source-url="#" origin-url="#">
     * </origin-iframe-genericmodal>
     */
    angular.module('origin-components')
        .directive('originIframeGenericmodal', originIframeGenericmodal);
}());
