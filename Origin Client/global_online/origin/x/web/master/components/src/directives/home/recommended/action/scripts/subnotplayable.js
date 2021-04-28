/**
 * @file home/recommended/Subnotplayable.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-subnotplayable';

    function OriginHomeRecommendedActionSubnotplayableCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionSubnotplayable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                imageRaw: '@image',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                offerId: '@offerid',
                ctaText: '@ctatext',
                ctaUrl: '@ctaurl',
                linkText: '@linktext',
                linkUrl: '@linkurl',
                priceOfferId: '@priceofferid'
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
     * @param {ImageUrl|OCD} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedText} description the description for the tile
     * @param {LocalizedString} ctatext cta text for the button
     * @param {Url} ctaurl url triggered when button is pushed (will direct to another page on the site)
     * @param {LocalizedString} linktext text for the link
     * @param {Url} linkurl link url
     * @param {OfferId} offerid the offerid of the game you want to interact with
     * @param {OfferId|OCD} priceofferid the offerid of the game you want to show pricing for

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