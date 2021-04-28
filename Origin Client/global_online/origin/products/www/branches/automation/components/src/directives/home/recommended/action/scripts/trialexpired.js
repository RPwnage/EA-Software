/**
 * @file home/recommended/trialexpired.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionTrialExpiredCtrl($scope, $controller, $sce, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-home-recommended-action-trial-expired';

        $scope.showTrialViolator = false;
        $scope.displayPrice = true;

        /**
         * custom description setup used for trial RNA passed into the shared controller
         */
        function customDescriptionFunction() {
            $scope.description = UtilFactory.getLocalizedStr($scope.descriptionRaw, CONTEXT_NAMESPACE, 'description');
        }

        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: customDescriptionFunction
        });

    }

    function originHomeRecommendedActionTrialExpired(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                gamename: '@gamename',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                priceOfferId: '@priceofferid',
                priceOcdPath: '@',
                offerId: '@offerid',
                ocdPath: '@',
                discoverTileImage: '@',
                discoverTileColor: '@',
                sectionTitle: '@',
                sectionSubtitle: '@'
            },
            controller: 'OriginHomeRecommendedActionTrialExpiredCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionTrialExpired
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} gamename the name of the game, if not passed it will use the name from catalog
     * @param {LocalizedString} subtitle the subtitle string
     * @param {LocalizedString} description the description string
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the ocd path of the game you want to interact with
     * @param {string} priceofferid the offerid of the game you want to show pricing for
     * @param {OcdPath} price-ocd-path the ocd path of the game you want to show pricing for
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     *
     * @description
     *
     * trial recommended next action (expired)
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-home-recommended-action-trial-expired offerid="OFB-EAST:109552444"></origin-home-recommended-action-trial-expired>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionTrialExpiredCtrl', OriginHomeRecommendedActionTrialExpiredCtrl)
        .directive('originHomeRecommendedActionTrialExpired', originHomeRecommendedActionTrialExpired);
}());