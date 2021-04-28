/**
 * @file home/recommended/Nonsubnotrecentlyplayed.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionNonsubnotrecentlyplayedCtrl($scope) {
        $scope.offerBased = false;
    }

    function originHomeRecommendedActionNonsubnotrecentlyplayed(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                subtitle: '@subtitle',
                description: '@description',
                offerId: '@offerid',
                ctaText: '@linktext'
            },
            controller: 'OriginHomeRecommendedActionNonsubnotrecentlyplayedCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionNonsubnotrecentlyplayed
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString|OCD} subtitle the subtitle for the time
     * @param {LocalizedString} description the description for the tile
     * @param {LocalizedString} linktext text for the link
     * @param {string} offerid the offerid for the tile
     * @description
     *
     * non sub no recently played recommended next action
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-nonsubnotrecentlyplayed image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_kingdomsofamalur_long.png" subtitlestr="Play for Free!" descriptionstr="Check out our selection of free games and get into the game." linkurl="#" linktextstr="Get a Free Game"></origin-home-recommended-action-nonsubnotrecentlyplayed>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionNonsubnotrecentlyplayedCtrl', OriginHomeRecommendedActionNonsubnotrecentlyplayedCtrl)
        .directive('originHomeRecommendedActionNonsubnotrecentlyplayed', originHomeRecommendedActionNonsubnotrecentlyplayed);
}());