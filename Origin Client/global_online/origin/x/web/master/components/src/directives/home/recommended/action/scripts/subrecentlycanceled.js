/**
 * @file home/recommended/Subrecentlycanceled.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-subrecentlycanceled';

    function OriginHomeRecommendedActionSubrecentlycanceledCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionSubrecentlycanceled(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                imageRaw: '@image',
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                ctaText: '@ctatext',
                ctaUrl: '@ctaurl'
            },
            controller: 'OriginHomeRecommendedActionSubrecentlycanceledCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/recommended/action/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeRecommendedActionSubrecentlycanceled
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl} image the link to the image for the tile, if not passed in it will use a default image
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedText} description the description for the tile
     * @param {LocalizedString} ctatext cta text for the button
     * @param {Url} ctaurl url triggered when button is pushed (will direct to another page on the site)
     * @description
     *
     * sub recently canceled recommended next action
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-recommended-action-subrecentlycanceled image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/tile_chinarising_long.png" titlestr="Just Added! Battlefield 4 Premium" subtitlestr="Get Back In The Game!" descriptionstr="Renew your subscription now to get access to more than 30 great games.">
     *         </origin-home-recommended-action-subrecentlycanceled>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginHomeRecommendedActionSubrecentlycanceledCtrl', OriginHomeRecommendedActionSubrecentlycanceledCtrl)
        .directive('originHomeRecommendedActionSubrecentlycanceled', originHomeRecommendedActionSubrecentlycanceled);
}());