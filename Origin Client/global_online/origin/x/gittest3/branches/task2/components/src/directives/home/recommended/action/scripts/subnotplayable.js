/**
 * @file home/recommended/Subnotplayable.js
 */
(function() {
    'use strict';

    function OriginHomeRecommendedActionSubnotplayableCtrl($scope) {
        $scope.offerBased = false;
    }

    function originHomeRecommendedActionSubnotplayable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                image: '@',
                subtitle: '@subtitle',
                description: '@description',
                offerId: '@offerid',
                linkUrl: '@href',
                ctaText: '@linktext'

            },
            controller: 'OriginHomeRecommendedActionSubnotplayableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionSubnotplayable
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image image of the tile
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedText} description the description for the tile
     * @param {LocalizedString} linktext cta text for the link
     * @param {Url} href link url
     * @param {string} offerid offer id on the tile
     * @description
     *
     * sub not playable recommended next action
     *
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-subnotplayable image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_crusader_long.png" subtitlestr="Explore the Vault!" descriptionstr="Check out all of the games available as part of your subscription." linkurl="#" linktextstr="Learn More">
     *         </origin-home-recommended-action-subnotplayable>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionSubnotplayableCtrl', OriginHomeRecommendedActionSubnotplayableCtrl)
        .directive('originHomeRecommendedActionSubnotplayable', originHomeRecommendedActionSubnotplayable);
}());