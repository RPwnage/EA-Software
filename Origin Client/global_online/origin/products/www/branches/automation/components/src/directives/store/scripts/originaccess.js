/**
 * @file /store/scripts/originaccess.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-origin-access';

    function OriginStoreOriginAccessCtrl($scope, UtilFactory, PriceInsertionFactory, SubscriptionFactory, NavigationFactory) {

        $scope.strings = {
            header: UtilFactory.getLocalizedStr($scope.header, CONTEXT_NAMESPACE, 'header'),
            href: UtilFactory.getLocalizedStr($scope.href, CONTEXT_NAMESPACE, 'href'),
            learnmore: UtilFactory.getLocalizedStr($scope.learnmore, CONTEXT_NAMESPACE, 'learnmore')
        };

        PriceInsertionFactory
            .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope.content, CONTEXT_NAMESPACE, 'content');

        $scope.$watch('strings.content', function(newValue, oldValue) {
            if(newValue !== oldValue) {
                $scope.infobubbleContent = '<strong>' + $scope.strings.header + '</strong>' +
                                            '<br>' + $scope.strings.content + '<br>' +
                                            '<a href="' + $scope.strings.href + '" class="otka">' + $scope.strings.learnmore + '</a>';
            }
        });

        $scope.onClick = function() {
            if(!SubscriptionFactory.userHasSubscription()) {
                NavigationFactory.openLink($scope.strings.href);
            }
        };

        /* This stops the infobubble-show event from making it to the store-game-tile */
        function stopPropagation(event) {
            event.stopPropagation();
        }

        $scope.$on('infobubble-show', stopPropagation);
        $scope.$on('infobubble-hide', stopPropagation);
    }

    function originStoreOriginAccess(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                hasInfobubble: '@hasinfobubble',
                header: '@',
                content: '@',
                href: '@',
                learnmore: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/originaccess.html'),
            controller: OriginStoreOriginAccessCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreOriginAccess
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} hasinfobubble if we should show the info bubble.
     * @param {LocalizedString} header the header to show in the info bubble.
     * @param {LocalizedTemplateString} content the content to show in the info bubble.
     * @param {LocalizedString} learnmore the cta button text.
     * @param {Url} href the url you want the learn more button to load (defaults to access landing page)
     * @description
     *
     * Origin Access info bubble
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-origin-access
     *              hasinfobubble="true">
     *         </origin-store-origin-access>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreOriginAccessCtrl', OriginStoreOriginAccessCtrl)
        .directive('originStoreOriginAccess', originStoreOriginAccess);
}());
