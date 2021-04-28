/**
 * @file store/about/scripts/downloadoriginnav.js
 */
(function () {
    'use strict';

    var EVENT_NAME = 'aboutDownloadOriginAnchor:added';
    var CONTEXT_NAMESPACE = 'origin-store-about-downloadorigin-nav';

    function originStoreAboutDownloadoriginNav(ComponentsConfigFactory, QueueFactory, UtilFactory) {

        function originStoreAboutDownloadoriginNavLink(scope) {

            scope.downloadOrigin = UtilFactory.getLocalizedStr(scope.downloadOriginText, CONTEXT_NAMESPACE, 'download-origin-text');
            scope.downloadOriginLink = null;

            function registerAnchor(anchor) {
                scope.downloadOriginLink = anchor;
            }

            var unfollowQueue = QueueFactory.followQueue(EVENT_NAME, registerAnchor);

            scope.$on('$destroy', unfollowQueue);
        }

        return {
            restrict: 'E',
            scope: {
                downloadOriginText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/downloadoriginnav.html'),
            link: originStoreAboutDownloadoriginNavLink,
            controller: 'originStoreAboutNavCtrl',
            replace: true
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutDownloadoriginNav
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} download-origin-text (optional) Download origin text. default is Download Origin
     * @description Creates a navigation menu for about page
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-downloadorigin-nav download-origin-text=""></origin-store-about-downloadorigin-nav>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAboutDownloadoriginNav', originStoreAboutDownloadoriginNav);
}());
