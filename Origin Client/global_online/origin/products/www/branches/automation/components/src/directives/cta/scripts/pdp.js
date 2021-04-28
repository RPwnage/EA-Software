/**
 * @file cta/pdp.js
 */

(function() {
    'use strict';

    var CLS_ICON_LEARNMORE = 'otkicon-learn-more';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    function OriginCtaPdpCtrl($scope, PdpUrlFactory, ComponentsLogFactory, ObjectHelperFactory, GamesDataFactory) {

        var catalogInfo = null;

        $scope.model = {};
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        function handleError(error) {
            ComponentsLogFactory.error('[origin-cta-pdp]', error);
        }

        function setCatalogInfo(catalogResponse) {
            catalogInfo = _.first(_.values(catalogResponse));
            $scope.btnHref = PdpUrlFactory.getPdpUrl(catalogInfo, true);
        }

        $scope.onBtnClick = function() {
            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked');
        };

        //offerid can start out as empty string and gets populated asynchronously
        var stopWatching = $scope.$watchGroup(['ocdPath', 'offerId'], function(newValues) {
            //watch until we get valid values
            if(newValues[0] || newValues[1]) {
                stopWatching();
                GamesDataFactory.lookUpOfferIdIfNeeded(newValues[0], newValues[1])
                    .then(ObjectHelperFactory.toArray)
                    .then(GamesDataFactory.getCatalogInfo)
                    .then(setCatalogInfo)
                    .catch(handleError);
            }
        });
    }

    function originCtaPdp(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                ocdPath: '@',
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaPdpCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaPdp
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {offerId} offer-id the ID of the offer
     * @param {OcdPath} ocd-path - ocd path of the offer to go to the pdp (ignored if offer id entered)
     *
     * @param {LocalizedString} learnmore *Update in the defaults
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-pdp offer-id="OFFER-123" description="Take me to PDP" type="primary"></origin-cta-pdp>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaPdpCtrl', OriginCtaPdpCtrl)
        .directive('originCtaPdp', originCtaPdp);
}());