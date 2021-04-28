/**
 * @file home/discovery/tile/programmable.js
 */
(function() {
    'use strict';

    function OriginHomeDiscoveryTileProgrammableCtrl($scope, $sce) {
        //this string will already be localized as it must always come from CQ5
        $scope.description = $sce.trustAsHtml($scope.descriptionString);
    }

    function originHomeDiscoveryTileProgrammable(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                art: '@image',
                descriptionString: '@description'
            },
            controller: 'OriginHomeDiscoveryTileProgrammableCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/discovery/tile/views/programmable.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeDiscoveryTileProgrammable
     * @restrict E
     * @element ANY
     * @scope
     * @param {ImageUrl|OCD} image the image to use
     * @param {LocalizedText} description the description text
     * @description
     *
     * programmable discovery tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-discovery-tile-programmable image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/discovery/programmed/tile_sims_4_48_hr_blog.jpg" description="<strong>SIMS 4 GAME TIME</strong><br>Are you ready to rule? The Sims 4 is now available with Origin Game Time, giving players 48 hours of free, unrestricted gameplay.">
     *             <origin-cta-directacquisition actionid="direct-acquistion" href="pathtogamelibrary" description="Get It Now" type="primary"></origin-cta-directacquisition>
     *             <origin-cta-loadurlexternal actionid="url-external" href="https://www.origin.com/en-us/news/the-sims-4-game-time" description="Read More" type="secondary"></origin-cta-loadurlexternal>
     *         </origin-home-discovery-tile-programmable>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeDiscoveryTileProgrammableCtrl', OriginHomeDiscoveryTileProgrammableCtrl)
        .directive('originHomeDiscoveryTileProgrammable', originHomeDiscoveryTileProgrammable);
}());
