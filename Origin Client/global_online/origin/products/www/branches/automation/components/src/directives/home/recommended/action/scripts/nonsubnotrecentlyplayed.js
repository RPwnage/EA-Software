/**
 * @file home/recommended/Nonsubnotrecentlyplayed.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-home-recommended-action-nonsubnotrecentlyplayed';

    function OriginHomeRecommendedActionNonsubnotrecentlyplayedCtrl($scope, $controller) {
        //instantiate the shared controller, but pass in the specific CONTEXT_NAMESPACE and subtitle/description function (if needed)
        $controller('OriginHomeRecommendedActionCtrl', {
            $scope: $scope,
            contextNamespace: CONTEXT_NAMESPACE,
            customSubtitleAndDescriptionFunction: null
        });
    }

    function originHomeRecommendedActionNonsubnotrecentlyplayed(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                subtitleRaw: '@subtitle',
                descriptionRaw: '@description',
                offerId: '@offerid',
                ctaText: '@ctatext',
                ctaUrl: '@ctaurl',
                linkText: '@linktext',
                linkUrl: '@linkurl',
                ocdPath: '@ocdPath',
                discoverTileImage: '@',
                discoverTileColor: '@',
                sectionTitle: '@',
                sectionSubtitle: '@'
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
     * @param {LocalizedString} subtitle the subtitle for the tile
     * @param {LocalizedText} description the description for the tile
     * @param {LocalizedString} ctatext cta text for the button
     * @param {Url} ctaurl url triggered when button is pushed (will direct to another page on the site)
     * @param {LocalizedString} linktext text for the link
     * @param {Url} linkurl link url
     * @param {string} offerid the offerid of the game you want to interact with
     * @param {OcdPath} ocd-path the ocd path of the game you want to interact with
     * @param {ImageUrl} discover-tile-image tile image 1000x250
     * @param {string} discover-tile-color the background color
     * @param {LocalizedString} section-title the text to show in the area title
     * @param {LocalizedString} section-subtitle the text to show in the area subtitle
     *
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
