/**
 * @file home/merchandise/tile.js
 */
(function() {
    'use strict';

    /**
     * Home Merchandise Tile directive
     * @directive originHomeMerchandiseTile
     */

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    var CONTEXT_NAMESPACE = 'origin-home-merchandise-tile';

    function OriginHomeMerchandiseTileCtrl($scope, $attrs, UtilFactory, PriceInsertionFactory) {
        $scope.showpricing = $attrs.showpricing && ($attrs.showpricing === BooleanEnumeration.true);
        $scope.isTouchDisable = !UtilFactory.isTouchEnabled();
        $scope.strings = {};

        function buildCalloutHtml(calloutText) {
            return calloutText ? '<span class="callout-text">' + calloutText + '</span>' : '';
        }

        function retrievePrice(field){
            var replacementObject = {
                '%callout-text%': buildCalloutHtml($scope.calloutText)
            };

            if ($scope.useDynamicPriceCallout){
                PriceInsertionFactory.insertPriceIntoLocalizedStr($scope, $scope.strings, $scope[field], CONTEXT_NAMESPACE, field, replacementObject);
            }else if ($scope.useDynamicDiscountPriceCallout){
                PriceInsertionFactory
                    .insertDiscountedPriceIntoLocalizedStr($scope, $scope.strings, $scope[field], CONTEXT_NAMESPACE, field, replacementObject);
            }else if ($scope.useDynamicDiscountRateCallout){
                PriceInsertionFactory
                    .insertDiscountedPercentageIntoLocalizedStr($scope, $scope.strings, $scope[field], CONTEXT_NAMESPACE, field, replacementObject);
            }else{
                PriceInsertionFactory
                    .insertPriceIntoLocalizedStr($scope, $scope.strings, $scope[field], CONTEXT_NAMESPACE, field, replacementObject);
            }
        }

        retrievePrice('text');
        retrievePrice('headline');
        /**
         * Send telemetry when the cta is clicked
         */
        function onCtaClicked(event, format) {
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web',
                bucketTitle = _.get($scope, ['$parent', '$parent', 'titleStr']) || _.get($scope, ['$parent', '$parent', '$parent', 'titleStr']) || '';

            format = format? format: 'live_tile';

            event.stopPropagation();
            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'message', source, 'success', {
                'type': 'in-game',
                'service': 'originx',
                'format': format,
                'client_state': 'foreground',
                'msg_id': $scope.ctid? $scope.ctid: $scope.image,
                'status': 'click',
                'content': $scope.text,
                'destination_name': bucketTitle
            });
        }

        // We listen for an event that is emitted by the transcluded cta directive, to trigger telemetry on cta click.
        $scope.$on('cta:clicked', onCtaClicked);
    }

    function originHomeMerchandiseTile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                ctid: '@',
                image: '@imagepath',
                headline: '@',
                text: '@',
                calloutText: '@',
                useDynamicPriceCallout: '@',
                useDynamicDiscountPriceCallout: '@',
                useDynamicDiscountRateCallout: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/merchandise/views/tile.html'),
            controller: OriginHomeMerchandiseTileCtrl
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeMerchandiseTile
     * @restrict E
     * @$element ANY
     * @$scope
     * @param {String} ctid Campaign Targeting ID
     * @param {ImageUrl} imagepath image in background
     * @param {LocalizedString} headline title text
     * @param {LocalizedString} text description text
     * @param {BooleanEnumeration} showpricing show pricing information
     * @param {LocalizedString} callout-text text that will be replaced into placeholder %callout-text% in headline or text
     *
     * @description
     *
     * merchandise tile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-merchandise-tile headline="Go All In. Save 30% On Premium." text="Upgrade to all maps, modes, vehicles and more for one unbeatable price." imagepath="/content/dam/originx/web/app/games/batman-arkham-city/181544/tiles/tile_batman_arkham_long.jpg" alignment="right" theme="dark" class="ng-isolate-$scope" ctid="campaing-id-1"></origin-home-merchandise-tile>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeMerchandiseTileCtrl', OriginHomeMerchandiseTileCtrl)
        .directive('originHomeMerchandiseTile', originHomeMerchandiseTile);

}());