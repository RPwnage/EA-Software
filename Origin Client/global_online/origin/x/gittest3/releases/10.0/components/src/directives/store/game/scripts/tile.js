/**
 * @file /store/game/scripts/tile.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-tile';

    /**
    * The controller
    */
    function OriginStoreGameTileCtrl($scope, GamesDataFactory, UtilFactory, ComponentsLogFactory, DialogFactory) {

        /* Set the tile type */
        $scope.type = $scope.type || TileTypeEnumeration.fixed;

        /* Set loading used for directive class */
        $scope.isLoading = true;

        /* Get localized strings */
        $scope.othString =  UtilFactory.getLocalizedStr($scope.othPriceStr, CONTEXT_NAMESPACE, 'othstr');

        function onGameData(data){
            /* This will come from OCD Feed */
            $scope.title = data[$scope.offerId].displayName;
            $scope.img = data[$scope.offerId].packArt;
            $scope.isOth = data[$scope.offerId].oth;
            $scope.pdpHref = "/#/store";
            $scope.violatorStr = $scope.isOth ? $scope.othString : $scope.violatorStr;

            $scope.isLoading = false;
            $scope.$digest();
        }

        /* Get catalog data by offerId */
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(function(data){
                if(!$.isEmptyObject(data)) {
                    onGameData(data);
                }
            })
            .catch(function(error) {
                Origin.log.exception(error, 'origin-store-game-tile - getGameInfo');
            });

        $scope.onOverlayWrapperClick = function() {
            ComponentsLogFactory.log('store-game-tile: Going to PDP');

            /* TODO: Update to pdp url when we have one */
            DialogFactory.openAlert({
                id: 'web-going-to-pdp-warning',
                title: 'Going to PDP',
                description: 'When the rest of OriginX is fully functional, you will be taken to the games pdp page.',
                rejectText: 'OK'
            });
        };
    }
    /**
    * The directive
    */
    function originStoreGameTile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: "@offerid",
                type: "@",
                violatorStr: "@violatorstr",
                othStr: "@othstr"
            },
            controller: 'OriginStoreGameTileCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/game/views/tile.html'),
        };
    }

    /**
     * A list of tile type options
     * @readonly
     * @enum {string}
     */
    var TileTypeEnumeration = {
        "fixed": "fixed",
        "responsive": "responsive",
        "compact": "compact"
    };

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id of the product
     * @param {TileTypeEnumeration} type can be one of three values 'fixed'/'responsive'/'compact'. this sets the style of the tile
     * @param {LocalizedString|OCD} title the title of the offer
     * @param {ImageUrl|OCD} img the pack art for the offer
     * @param {string|OCD} pdphref the URL to the PDP for the offer
     * @param {LocalizedString|OCD} othstr the violator text if offer is OTH.
     * @description
     *
     * Store Game Tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-game-tile
     *              offerid="Origin.OFR.50.0000675"
     *              type="fixed">
     *          </origin-store-game-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameTileCtrl', OriginStoreGameTileCtrl)
        .directive('originStoreGameTile', originStoreGameTile);
}());
