/**
 * @file cta/directacquistion.js
 */

(function () {
    'use strict';

    /**
     * BooleanEnumeration
     * @enum {string}
     */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-cta-directacquisition';

    function OriginCtaDirectacquisitionCtrl($scope, PurchaseFactory, UtilFactory, ProductFactory, PdpUrlFactory, ComponentsLogFactory, GamesDataFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.btnText = UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description');

        $scope.$watchOnce('offerId', function(offerId){
            ProductFactory.get(offerId).attachToScope($scope, 'model');
        });

        $scope.$watchOnce('model', function(model) {
            $scope.btnHref = PdpUrlFactory.getPdpUrl(model, true);
        });

        $scope.onBtnClick = function ($event) {
            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked');

            // Emit an event to remove any info bubble that may be showing
            $scope.$emit('infobubble-remove');

            $event.stopPropagation();
            $event.preventDefault();

            if ($scope.offerId && $scope.model) {
                if (UtilFactory.isTouchEnabled() && !PdpUrlFactory.isPdpRoute()) {
                    PdpUrlFactory.goToPdp($scope.model);
                } else {
                    if ($scope.isVault === BooleanEnumeration.true) {
                        PurchaseFactory.entitleVaultGame($scope.offerId);
                    } else {
                        // and explicitly passing along the offer ID
                        PurchaseFactory.entitleFreeGame($scope.offerId);
                    }
                }
            }
        };

        /* this is used to stop propagation, if this lives inside of a clickable area */
        $scope.onInfobubbleClick = function ($event) {
            $event.stopPropagation();
        };

        function handleError(error) {
            ComponentsLogFactory.error('[origin-cta-directacquisition]', error);
        }

        function setOfferId(offerId) {
            $scope.offerId = offerId;
        }

        GamesDataFactory.lookUpOfferIdIfNeeded($scope.ocdPath, $scope.offerId)
            .then(setOfferId)
            .catch(handleError);
    }

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

    function originCtaDirectacquisition(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                description: '@',
                type: '@',
                showInfoBubble: '@showinfobubble',
                offerId: '@',
                isVault: '@',
                ocdPath: '@'
            },
            controller: 'OriginCtaDirectacquisitionCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaDirectacquisition
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text overrides the default text 'get it now'
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {BooleanEnumeration} showinfobubble - If info bubble with ratings is shown.
     * @param {String} offer-id the offer id for the CTA
     * @param {BooleanEnumeration} is-vault true if offer is a vault game, otherwise optional/false
     * @param {OcdPath} ocd-path - ocd path of the offer to acquire (ignored if offer id entered)
     * @param {LocalizedString} learnmore *Update defaults to change
     * @description
     * Direct Acquistion call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-directacquisition
     *             description="Entitle Me To Free Game"
     *             type="primary"
     *             showinfobubble="true"
     *             offer-id="OFFER-123"
     *             is-vault="true">
     *         </origin-cta-directacquisition>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaDirectacquisitionCtrl', OriginCtaDirectacquisitionCtrl)
        .directive('originCtaDirectacquisition', originCtaDirectacquisition);
}());
