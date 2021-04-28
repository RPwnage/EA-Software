/**
 * @file /store/scripts/originaccess.js
 */

(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-originaccess';

    function OriginStoreOriginAccessCtrl($scope, $sce, UtilFactory, $location) {

        /* Get localized strings */
        $scope.href = "#/store/originaccess";
        $scope.InfobubbleContent = UtilFactory.getLocalizedStr($scope.InfobubbleContent, CONTEXT_NAMESPACE, 'infobubblecontentstr');
        $scope.InfobubbleContent = $sce.trustAsHtml($scope.InfobubbleContent);

        $scope.onClick = function()
        {
            $location.path($scope.href);
        };
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