/**
 * @file /store/game/scripts/rating.js
 */
(function() {
    'use strict';

    function OriginStoreGameRatingCtrl($scope, GamesDataFactory) {

        /* TODO: Replace this with real data when whe have it */
        var tempData = {
            esrb: {
                ratingSystem: "ESRB",
                ESRBPendingMature: "false",
                ratingSystemIconESRB: "https://eaassets-a.akamaihd.net/origin-com-store-final-assets-prod/ratings-icon/esrb/ESRB_M.png",
                gameRatingUrlESRB: "http://www.esrb.org/",
                gameRatingDescriptionLongESRB: "Online Interactions Not Rated by the ESRB",
                gameRatingDescESRB01: "Blood and Gore",
                gameRatingDescESRB02: "Intense Violence",
                gameRatingDescESRB03: "Strong Language",
                gameRatingESRB: "Mature"
            },
            pegi: {
                PEGIPendingMature: "false"
            }
        };

        function onGameData(data) {

            var ratingData;

            /* Inject the temp data */
            /* TODO: Replace this with real data when whe have it */
            data[$scope.offerId].rating = tempData;

            ratingData = data[$scope.offerId].rating.esrb;

            $scope.rating = ratingData.gameRatingESRB;
            $scope.ratingSystem = ratingData.ratingSystem;
            $scope.ratingSystemIcon = ratingData.ratingSystemIconESRB;
            $scope.ratingUrl = ratingData.gameRatingUrlESRB;
            $scope.ratingFootnote = ratingData.gameRatingDescriptionLongESRB;
            $scope.ratingDescriptors = [];
            $scope.ratingDescriptors.push(ratingData.gameRatingDescESRB01);
            $scope.ratingDescriptors.push(ratingData.gameRatingDescESRB02);
            $scope.ratingDescriptors.push(ratingData.gameRatingDescESRB03);

            $scope.$digest();
        }

        /* Get catalog data by offerId */
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(onGameData)
            .catch(function(error) {
                Origin.log.exception(error, 'origin-store-rating - getGameInfo');
            });
    }

    function originStoreGameRating(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@offerid'
            },
            controller: 'OriginStoreGameRatingCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/game/views/rating.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameRating
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid The offer id for the ratings and legal text
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-game-rating offerid="OFFER-123"></origin-store-game-rating>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameRatingCtrl', OriginStoreGameRatingCtrl)
        .directive('originStoreGameRating', originStoreGameRating);
}());