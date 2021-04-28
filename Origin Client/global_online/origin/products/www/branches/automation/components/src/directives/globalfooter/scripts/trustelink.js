
/**
 * @file globalfooter/scripts/trustelink.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-globalfooter-truste-link';

    function originGlobalfooterTrusteLink(ComponentsConfigFactory, UtilFactory) {

        function originGlobalfooterTrusteLinkLink(scope) {
            scope.visible = !Origin.client.isEmbeddedBrowser() && window.hasOwnProperty('truste');
            scope.linkText = UtilFactory.getLocalizedStr(scope.linkText, CONTEXT_NAMESPACE, 'link-url');

            scope.onLinkClick = function () {
                if (window.truste.hasOwnProperty('eu')) {
                    window.truste.eu.clickListener();
                }
            };
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                linkText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('globalfooter/views/trustelink.html'),
            link: originGlobalfooterTrusteLinkLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalfooterTrusteLink
     * @restrict E
     * @element ANY
     * @scope
     * @description Provisions a simple link in the global footer
     * @param {LocalizedText} link-text The text for the footer link ("Cookie Preferences")
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-globalfooter-truste-link />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGlobalfooterTrusteLink', originGlobalfooterTrusteLink);
}());
