/**
 * @file /store/game/scripts/tile.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-tile';

    function OriginStoreGameTileCtrl($scope, $location, UtilFactory, ProductFactory, PdpUrlFactory) {
        $scope.type = $scope.type || TileTypeEnumeration.fixed;

        $scope.strings = {
            othText: UtilFactory.getLocalizedStr($scope.othText, CONTEXT_NAMESPACE, 'othstr')
        };

        $scope.goToPdp = PdpUrlFactory.goToPdp;

        $scope.getViolatorText = function() {
            return $scope.model.oth ? $scope.strings.othText : $scope.violatorText;
        };

        $scope.model = {};
        var removeWatcher = $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                removeWatcher();
                ProductFactory.get(newValue).attachToScope($scope, 'model');
            }
        });
    }

    function originStoreGameTile(ComponentsConfigFactory) {

        function originStoreGameTileLink(scope, element) {

            /* This adds a hover class when the infobubble is hovered */
            scope.$on('infobubble-show', function() {
                element.addClass('origin-storegametile-hover');
            });

            /* this removes the class */
            scope.$on('infobubble-hide', function() {
                element.removeClass('origin-storegametile-hover');
            });

        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: "@",
                type: "@",
                violatorText: "@",
                othText: "@"
            },
            link: originStoreGameTileLink,
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
     * @param {OfferId} offer-id the offer id
     * @param {TileTypeEnumeration} type can be one of three values 'fixed'/'responsive'/'compact'. this sets the style of the tile
     * @param {LocalizedString|OCD} oth-text the violator text if offer is OTH.
     * @param {LocalizedString|OCD} violator-text the violator text.
     * @description
     *
     * Store Game Tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-game-tile
     *              offer-id="Origin.OFR.50.0000675"
     *              type="fixed">
     *          </origin-store-game-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameTileCtrl', OriginStoreGameTileCtrl)
        .directive('originStoreGameTile', originStoreGameTile);
}());
