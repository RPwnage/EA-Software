(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePageGamesGametileCtrl($scope, UtilFactory, ProductNavigationFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-page-games-gametile';

        $scope.learnMoreLoc = UtilFactory.getLocalizedStr($scope.learnMoreStr, CONTEXT_NAMESPACE, 'learn-more-str');
        $scope.detailsLoc = UtilFactory.getLocalizedStr($scope.detailsStr, CONTEXT_NAMESPACE, 'details-str');

        /**
         * Navigate to OGD or PDP depending upon the ownership
         * @method navigateToGame
         */
        $scope.navigateToGame = function() {
            if($scope.offerId) {
                ProductNavigationFactory.goProductDetails($scope.offerId);
            }
        };

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePageGamesGametile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @param {string} titlestr the title of the game
     * @param {string} image the box art image URL
     * @param {LocalizedString} details-str "Details"
     * @param {LocalizedString} learn-more-str "Learn More"
     * @description
     *
     * Profile Page - Simple pack art game tile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-page-games-gametile
     *             image="{imageURL}"
     *             titlestr="{game name}"
     *             offerid="{offerId}"
     *             learnmorestr="Learn More"
     *             detailsstr="Details"
     *         ></origin-profile-page-games-gametile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePageGamesGametileCtrl', OriginProfilePageGamesGametileCtrl)
        .directive('originProfilePageGamesGametile', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePageGamesGametileCtrl',
                scope: {
                    image: '@',
                    titlestr: '@',
                    offerId: '@offerid',
                    learnMoreStr: '@',
                    detailsStr: '@',
                    owned: '='
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/page/games/views/gametile.html')
            };

        });
}());

