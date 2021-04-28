/**
 * @file /store/scripts/cta.js
 */
(function() {
    'use strict';

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

    function OriginStoreCtaCtrl($scope, UtilFactory, GamesDataFactory, AuthFactory, SubscriptionFactory, ComponentsLogFactory) {

        var isSubscriber, isOriginAccessOffer, isFree, isOwned;

        /* Get localized strings */
        $scope.pdpDescriptionStr =  UtilFactory.getLocalizedStr($scope.description, 'origin-cta-pdp', 'learnmore');
        $scope.directacquisitionDescriptionStr =  UtilFactory.getLocalizedStr($scope.description, 'origin-cta-directacquisition', 'getitnow');

        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.isSubscriber = false;
        $scope.isLoading = true;
        $scope.isPdpCta = false;
        $scope.isEntitlementCta = false;

        $scope.infobubbleContent =  '<origin-store-game-rating offerid="{{offerId}}"></origin-store-game-rating>'+
                                    '<origin-store-game-legal offerid="{{offerId}}"></origin-store-game-legal>';

        function onGameData(data){

            isFree = (data[$scope.offerId].oth || data[$scope.offerId].trial || data[$scope.offerId].demo) ? true : false;
            isOriginAccessOffer = data[$scope.offerId].vault;

            if (AuthFactory.isAppLoggedIn()) {
                isOwned = GamesDataFactory.getEntitlement($scope.offerId) !== undefined;
                isSubscriber = SubscriptionFactory.userHasSubscription();
            }
            else {
                isSubscriber = false;
                isOwned = false;
            }

            $scope.isPdpCta = isOwned || !isOwned && !isFree && !(isSubscriber && isOriginAccessOffer);
            $scope.isEntitlementCta = !isOwned && isFree || (isSubscriber && isOriginAccessOffer && !isOwned);
            $scope.isLoading = false;

            $scope.$digest();
        }

        /* Get catalog data by offerId */
        /* This is faking the price service */
        GamesDataFactory.getCatalogInfo([$scope.offerId])
            .then(onGameData)
            .catch(function(error) {
                ComponentsLogFactory.error('origin-store-price - getGameInfo', error);
            });

        /* Update owned based on users entitlements */
        function onEntitlementUpdate() {
            /* TODO: update to ownsEntitlement when Mas checks it in */
            isOwned = GamesDataFactory.getEntitlement($scope.offerId) !== undefined;
            $scope.isLoading = false;
            $scope.$digest();
        }

        /* Bind to entitlement updates */
        GamesDataFactory.events.on('games:baseGameEntitlementsUpdate', onEntitlementUpdate);

        $scope.$on('$destroy', function () {
            GamesDataFactory.events.off('games:baseGameEntitlementsUpdate', onEntitlementUpdate);
        });
    }

    function originStoreCta(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid',
                href: '@',
                description: '@',
                type: '@',
                showInfoBubble: '@showinfobubble'
            },
            controller: 'OriginStoreCtaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/cta.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offerid the offer id for the CTA
     * @param {string} href the default link for the CTA
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @description
     *
     * Determines the CTA to used based of the offer ID
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-cta
     *          offerid="OFR.123"
     *          href="http://www.origin.com"
     *          description="Click Me!"
     *          type="primary">
     *         </origin-store-cta>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginStoreCtaCtrl', OriginStoreCtaCtrl)
        .directive('originStoreCta', originStoreCta);
}());