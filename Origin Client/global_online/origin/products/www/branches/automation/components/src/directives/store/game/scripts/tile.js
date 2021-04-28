/**
 * @file /store/game/scripts/tile.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-tile';
    var HOVER_CLASS = 'origin-storegametile-hover';

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

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStoreGameTile(ComponentsConfigFactory, UtilFactory, ProductFactory, OcdPathFactory, PdpUrlFactory, DirectiveScope, ScopeHelper, ProductInfoHelper) {

        function originStoreGameTileLink(scope, element) {
            scope.goToPdp = function(model, $event){
                $event.preventDefault();
                PdpUrlFactory.goToPdp(model);
            };
            
            scope.getPdpUrl = PdpUrlFactory.getPdpUrl;

            function setType() {
                scope.type = TileTypeEnumeration[scope.type] || TileTypeEnumeration.fixed;
            }

            function setTypeDigest(){
                setType();
                //If OCD response returns after digest cycle ends, tile will not be styled properly
                ScopeHelper.digestIfDigestNotInProgress(scope);
            }

            scope.strings = {
                othText: UtilFactory.getLocalizedStr(scope.othText, CONTEXT_NAMESPACE, 'oth-text')
            };

            /* This adds a hover class when the infobubble is hovered */
            scope.$on('infobubble-show', function() {
                element.addClass(HOVER_CLASS);
            });

            /* this removes the class */
            scope.$on('infobubble-hide', function() {
                element.removeClass(HOVER_CLASS);
            });

            scope.$watchOnce('model', function(){
                scope.violatorText = scope.model.oth ? scope.strings.othText : scope.violatorText;
                scope.isFree = ProductInfoHelper.isFree(scope.model);
            }, false, function(){
                return !_.isEmpty(scope.model);
            });

            scope.model = {};
            var stopWatchingOcdPath = scope.$watch('ocdPath', function (ocdPath) {
                if (ocdPath) {
                    stopWatchingOcdPath();
                    stopWatchingOfferId();
                    OcdPathFactory.get(ocdPath).attachToScope(scope, 'model');
                    DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE, ocdPath)
                        .then(setTypeDigest);
                }
            });

            var stopWatchingOfferId = scope.$watch('offerId', function (offerId) {
                if (offerId) {
                    stopWatchingOfferId();
                    stopWatchingOcdPath();
                    ProductFactory.get(offerId).attachToScope(scope, 'model');
                    setType();
                }
            });

        }

        return {
            restrict: 'E',
            scope: {
                ocdPath: '@',
                offerId: '@',
                type: '@',
                violatorText: '@',
                othText: '@',
                overrideVault: '@'
            },
            link: originStoreGameTileLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/game/views/tile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameTile
     * @restrict E
     * @element ANY
     * @scope
     * @param {OcdPath} ocd-path OCD Path
     * @param {OfferId} offer-id Offer Id used for store search
     * @param {TileTypeEnumeration} type can be one of three values 'fixed'/'responsive'/'compact'. this sets the style of the tile.
     * @param {LocalizedString|OCD} oth-text the violator text if offer is OTH.
     * @param {LocalizedString|OCD} violator-text the violator text.
     * @param {BooleanEnumeration} override-vault Override the "included with vault" text
     * @description
     *
     * Store Game Tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-game-tile
     *              ocd-path="/battlefield/battlefield-4/trial"
     *              type="fixed">
     *          </origin-store-game-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreGameTile', originStoreGameTile);
}());
