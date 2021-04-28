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

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE_PDP = 'origin-cta-pdp';
    var CONTEXT_NAMESPACE = 'origin-store-cta';
    
    function OriginStoreCtaCtrl($scope, UtilFactory, PdpFactory, ProductInfoHelper, UserObserverFactory, ProductFactory, GamesCatalogFactory, VaultRefiner, GameRefiner, GamesEntitlementFactory) {

        // The user owns this game outright, they did not get it through the vault
        function ownsOutright() {
            return GameRefiner.isOwnedOutright($scope.model);
        }

        /**
         * User owns this offer outright, or user is entitled to the vault edition of this offer.
         * Note if user ownsThisOfferThroughVault then by default the user ownsVaultEdition
         */
        function isEntitledToOffer() {
            return ownsOutright() || $scope.ownsVaultEdition;
        }

        // This is not a free or early access offer
        function isNotFreeOrEarlyAccess() {
            return !($scope.isFree || $scope.isEarlyAccess);
        }

        // the user is not allowed to be entitled to the game through the vault
        function vaultEntitlementNotAllowed() {
            return !($scope.user.isSubscriber && $scope.isOriginAccessOffer);
        }

        function userIsEntitlableSubscriber() {
            return !!($scope.user.isSubscriber && ($scope.couldBeEntitledThroughVault || $scope.isEarlyAccess));
        }

        function setCtaOfferId(offerId) {
            $scope.ctaOfferId = offerId;
        }

        /**
         * Sets ownsVaultEdition
         * @param offerId The offer id of the vault edition for this offer (could be the offer itself).
         * @returns {string} Offer id
         */
        function getVaultEditionOwnedInformation(vaultEditionOfferId) {
            // This means that the user owns the vault level entitlement. Ie. either this offer is a vault offer and user owns it,
            // or they own the vault offer that is the upgrade for this offer.
            $scope.ownsVaultEdition = GamesEntitlementFactory.isSubscriptionEntitlement(vaultEditionOfferId);
            $scope.couldBeEntitledThroughVault = $scope.couldBeEntitledThroughVault && !$scope.ownsVaultEdition;
            return vaultEditionOfferId;
        }

        // Sets up needed logic variables after model loads
        function onGameData(data){
            $scope.model = data;

            // link to PDP if the user owns a greater edition than the vault edition, hides entitlement button
            if(GameRefiner.isBaseGameOfferType(data) && $scope.user.isSubscriber && ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable)) {
                PdpFactory.getEditions($scope.model).then(function(response) {
                    $scope.ownsGreaterEdition = PdpFactory.ownsGreaterEdition(response, $scope.model);
                });
            } else {
                $scope.ownsGreaterEdition = false;
            }

            $scope.isFree = ProductInfoHelper.isFree(data);
            // This means that there is a vault game that is entitleable for this offer (either itself or there is a higher edition vault game that can be upgraded to)
            $scope.isOriginAccessOffer = !!($scope.model.vaultOcdPath && ($scope.model.subscriptionAvailable || $scope.model.isUpgradeable));
            $scope.isEarlyAccess = $scope.model.earlyAccess;
            $scope.isOwned = $scope.model.isOwned;
            $scope.isLoading = false;
            // This means that the game could be entitled through the vault instead of permanently entitled, as there is a vault offering available and it is not free
            $scope.couldBeEntitledThroughVault = $scope.isOriginAccessOffer && !$scope.isFree;
            $scope.isBrowserGame = GameRefiner.isBrowserGame($scope.model);
            // This means that the user owns the vault level entitlement to this offer and does not own the offer permanently
            $scope.ownsThisOfferThroughVault = $scope.model.vaultEntitlement;

            // If the offer should be entitled through the vault, find the vault edition that should be entitled
            if($scope.couldBeEntitledThroughVault) {
                // get offer id for vault offer
                GamesCatalogFactory
                    .getOfferIdByPath($scope.model.vaultOcdPath, !VaultRefiner.isShellBundle($scope.model))
                    .then(getVaultEditionOwnedInformation)
                    .then(setCtaOfferId);
            } else {
                setCtaOfferId($scope.model.offerId);
            }
        }

        /* Get localized strings */
        $scope.pdpDescriptionStr =  UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE_PDP, 'learnmore');
        $scope.directacquisitionDescriptionStr =  UtilFactory.getLocalizedStr($scope.description, 'origin-cta-directacquisition', 'description');
        $scope.learnMoreStr =  UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE_PDP, 'learnmore');
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.isLoading = true;
        $scope.model = {};
        UserObserverFactory.getObserver($scope, 'user');
        $scope.playNow = UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'play-now');

        $scope.infobubbleContent =  '<origin-store-game-rating></origin-store-game-rating>'+
            '<origin-store-game-legal></origin-store-game-legal>';

        // initialize variables
        $scope.isOwned = false;
        $scope.isEarlyAccess = false;
        $scope.isOriginAccessOffer = false;
        $scope.isBrowserGame = false;
        $scope.ownsThisOfferThroughVault = false;
        $scope.ownsVaultEdition = false;

        $scope.isPdpCta = function () {
            return $scope.offerId &&
                (isEntitledToOffer() || (isNotFreeOrEarlyAccess() && vaultEntitlementNotAllowed()) || $scope.ownsGreaterEdition);
        };

        $scope.isEntitlementCta = function () {
            return $scope.offerId &&
                !ownsOutright() &&
                !$scope.isBrowserGame &&
                (userIsEntitlableSubscriber() || ($scope.isFree && !$scope.isEarlyAccess)) &&
                !$scope.ownsGreaterEdition;
        };

        $scope.isLearnMore = function () {
            return $scope.offerId && !$scope.isPdpCta() && !$scope.isEntitlementCta() && !$scope.isBrowserGame;
        };

        $scope.$watchOnce('offerId', function(newValue) {
            ProductFactory.get(newValue).attachToScope($scope, onGameData);
        });
    }

    function originStoreCta(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@',
                href: '@',
                description: '@',
                type: '@',
                showInfoBubble: '@showinfobubble',
                showInfoBubbleOthLink: '@'
            },
            controller: 'OriginStoreCtaCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offer-id the offer id for the CTA
     * @param {string} href the default link for the CTA
     * @param {LocalizedString} description the button text
     * @param {BooleanEnumeration} showinfobubble show or hide info bubble
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {BooleanEnumeration} show-info-bubble-oth-link shows oth link in info bubble or not
     * @param {LocalizedString} play-now browser game cta text
     *
     * @description
     *
     * Determines the CTA to used based of the offer ID
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-cta
     *          offer-id="OFR.123"
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
