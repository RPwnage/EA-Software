/**
 * @file home/recommended/Nonsubnotplayable.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionNonsubnotplayableCtrl($scope) {
        $scope.offerBased = false;
    }

    function originHomeRecommendedActionNonsubnotplayable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@image',
                subtitle: '@subtitle',
                description: '@description',
                offerId: '@offerid',
                linkUrl: '@href',
                ctaText: '@linktext'
            },
            controller: 'OriginHomeRecommendedActionNonsubnotplayableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionNonsubnotplayable
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedText} description the description for the tile
     * @param {LocalizedString} linktext cta text for the link
     * @param {Url} href link url
     * @description
     *
     * non sub no playable games recommended next action
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-nonsubnotplayable image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_fifaworld_long.png" subtitlestr="Play For Free!" descriptionstr="Check out our selection of free games and get into the game." linkurl="#" linktextstr="Learn More"></origin-home-recommended-action-nonsubnotplayable>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionNonsubnotplayableCtrl', OriginHomeRecommendedActionNonsubnotplayableCtrl)
        .directive('originHomeRecommendedActionNonsubnotplayable', originHomeRecommendedActionNonsubnotplayable);
}());