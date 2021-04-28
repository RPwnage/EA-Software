/**
 * @file gamelibrary/ogdheader.js
 */
(function() {
    'use strict';

    function OriginGamelibraryOgdHeaderCtrl($scope, $sce, GamesDataFactory, ComponentsLogFactory) {
        // WIP: Add batlefield 4 branding and alternate video backdrop for DR:225064100 (BF3) until CMS is hooked up
        if($scope.offerId === 'DR:225064100') {
            $scope.backgroundVideoSrcFiles = [{
                    src: $sce.trustAsResourceUrl('https://dev.sb3.x.origin.com:4502/content/dam/originx/web/app/games/battlefield/76889/video/bf4_bg.mp4'),
                    type: 'video/mp4'
                }, {
                    src: $sce.trustAsResourceUrl('https://dev.sb3.x.origin.com:4502/content/dam/originx/web/app/games/battlefield/76889/video/bf4_bg.webm'),
                    type: 'video/webm'
                }];
            $scope.backgroundVideoPoster = 'https://dev.sb3.x.origin.com:4502/content/dam/originx/web/app/games/battlefield/76889/video/bf4-video-placeholder.jpg';
            $scope.gameLogo = 'https://dev.sb3.x.origin.com:4502/content/dam/originx/web/app/games/battlefield/76889/ogd/bf4.png';
            $scope.gamePremiumLogo = 'https://dev.sb3.x.origin.com:4502/content/dam/originx/web/app/games/battlefield/76889/ogd/bf4_premiumbadge.png';
            $scope.accessLogo = 'https://dev.sb3.x.origin.com:4502/content/dam/originx/web/app/games/battlefield/76889/ogd/originaccess.png';
        }

        function handleCatalogInfoResponse(response) {
            $scope.packartLarge = response[$scope.offerId].packArt;
            $scope.gameName = response[$scope.offerId].displayName;
            $scope.$digest();
        }

        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(handleCatalogInfoResponse)
            .catch(function(error) {
                ComponentsLogFactory.error('[origin-ogd-header] GamesDataFactory.getCatalogInfo', error.stack);
            });
    }

    function originGamelibraryOgdHeader(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('gamelibrary/views/ogdheader.html'),
            controller: OriginGamelibraryOgdHeaderCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGamelibraryOgdHeader
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id
     * @description
     *
     * Owned game details slide out header section
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-ogd>
     *             <origin-gamelibrary-ogd-header offerid="OFB-EAST:50885"></origin-gamelibrary-ogd-header>
     *         </origin-gamelibrary-ogd>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginGamelibraryOgdHeaderCtrl', OriginGamelibraryOgdHeaderCtrl)
        .directive('originGamelibraryOgdHeader', originGamelibraryOgdHeader);
}());