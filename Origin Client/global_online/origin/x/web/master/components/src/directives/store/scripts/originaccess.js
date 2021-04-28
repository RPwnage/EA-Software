/**
 * @file /store/scripts/originaccess.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-originaccess';

    function OriginStoreOriginAccessCtrl($scope, $sce, UtilFactory, $location) {

        /* Get localized strings */
        $scope.href = "#/store/originaccess";
        $scope.infobubbleContent = UtilFactory.getLocalizedStr($scope.infobubbleContent, CONTEXT_NAMESPACE, 'infobubblecontentstr');

        $scope.onClick = function() {
            $location.path($scope.href);
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
                infobubbleContent: '@infobubblecontent'
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
     * @param {LocalizedString} infobubblecontent the content to show in the info bubble.
     * @description
     *
     * Shows an avatar for a user
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-origin-access hasinfobubble="true" infobubblecontent="hello!"></origin-store-origin-access>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreOriginAccessCtrl', OriginStoreOriginAccessCtrl)
        .directive('originStoreOriginAccess', originStoreOriginAccess);
}());