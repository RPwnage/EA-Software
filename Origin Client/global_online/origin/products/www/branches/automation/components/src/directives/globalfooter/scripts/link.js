
/**
 * @file globalfooter/scripts/link.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-globalfooter-link';

    function originGlobalfooterLink(ComponentsConfigFactory, NavigationFactory, UtilFactory) {

        function originGlobalfooterLinkLink(scope) {

            scope.strings = {
                linkUrl: UtilFactory.getLocalizedStr(scope.linkUrl, CONTEXT_NAMESPACE, 'link-url')
            };
            scope.strings.linkUrlAbsolute = NavigationFactory.getAbsoluteUrl(scope.strings.linkUrl);

            scope.onLinkClick = function ($event) {
                $event.preventDefault();
                $event.stopPropagation();
                NavigationFactory.openLink(scope.strings.linkUrl);
            };
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                linkText: '@',
                linkUrl: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('globalfooter/views/link.html'),
            link: originGlobalfooterLinkLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originGlobalfooterLink
     * @restrict E
     * @element ANY
     * @scope
     * @description Provisions a simple link in the global footer
     * @param {LocalizedText} link-text The text for the footer link
     * @param {Url} link-url The url that this footer link will redirect to
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-globalfooter-link />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originGlobalfooterLink', originGlobalfooterLink);
}());
