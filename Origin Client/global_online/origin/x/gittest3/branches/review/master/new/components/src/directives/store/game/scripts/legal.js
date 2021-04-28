/**
 * @file /store/game/scripts/legal.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-legal';

    function OriginStoreGameLegalCtrl($scope, GamesDataFactory, $sce, UtilFactory) {

        /* Get localized strings */
        $scope.eulaStr =  $sce.trustAsHtml(UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'eulastr'));
        $scope.termsStr =  $sce.trustAsHtml(UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'termsstr'));
        $scope.othHeaderStr =  $sce.trustAsHtml(UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'othheaderstr'));
        $scope.othStr =  $sce.trustAsHtml(UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'othstr'));

        function onGameData(data) {
            $scope.isOth = data[$scope.offerId].oth;
            $scope.$digest();
        }

        /* Get catalog data by offerId */
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(onGameData)
            .catch(function(error) {
                Origin.log.exception(error, 'origin-store-rating - getGameInfo');
            });
    }

    function originStoreGameLegal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            controller: 'OriginStoreGameLegalCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/game/views/legal.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameLegal
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid The offer id for the ratings and legal text
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-game-rating offerid="OFFER-123"></origin-store-game-rating>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameLegalCtrl', OriginStoreGameLegalCtrl)
        .directive('originStoreGameLegal', originStoreGameLegal);
}());