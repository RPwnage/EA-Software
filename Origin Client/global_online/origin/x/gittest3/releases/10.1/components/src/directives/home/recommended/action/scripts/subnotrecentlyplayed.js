/**
 * @file home/recommended/subnotrecentlyplayed.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionSubnotrecentlyplayedCtrl($scope) {
        $scope.offerBased = false;
    }

    function originHomeRecommendedActionSubnotrecentlyplayed(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                subtitle: '@subtitle',
                description: '@description',
                offerId: '@offerid',
                ctaText: '@linktext'
            },
            controller: 'OriginHomeRecommendedActionSubnotrecentlyplayedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionSubnotrecentlyplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image image of the tile
     * @param {LocalizedString|OCD} subtitle subtile on the tile
     * @param {LocalizedString} description description on the tile
     * @param {LocalizedString} linktext link text str
     * @param {string} offerid offer id on the tile
     * @description
     *
     * sub not recently played recommended next action
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-subnotrecentlyplayed image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_sims4_long.png" subtitlestr="Check out the latest games in the vault" descriptionstr="Check out all of the games available as part of your subscription." linkurl="#" linktextstr="Go to the Vault">
     *         </origin-home-recommended-action-subnotrecentlyplayed>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionSubnotrecentlyplayedCtrl', OriginHomeRecommendedActionSubnotrecentlyplayedCtrl)
        .directive('originHomeRecommendedActionSubnotrecentlyplayed', originHomeRecommendedActionSubnotrecentlyplayed);
}());