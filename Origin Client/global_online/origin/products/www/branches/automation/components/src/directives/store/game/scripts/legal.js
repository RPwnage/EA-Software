/**
 * @file /store/game/scripts/legal.js
 */
/* global moment */
(function() {
    'use strict';

    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-store-game-legal',
        POLICY_PAGE_ROUTE = 'app.store.wrapper.promo-terms',
        ON_THE_HOUSE_ROUTE = 'app.store.wrapper.onthehouse';

    function OriginStoreGameLegalCtrl($scope, UtilFactory, PdpUrlFactory, AppCommFactory, $location, NavigationFactory) {
        $scope.strings = $scope.strings || {};
        $scope.strings.eulaText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'eula-text');
        $scope.strings.othText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'oth-text');
        $scope.strings.promoText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'promo-text');

        function populateScope() {
            var pdpUrl = PdpUrlFactory.getPdpUrl($scope.model);
            $scope.linkTerms = pdpUrl + '#terms';
            $scope.termsUrl = NavigationFactory.getAbsoluteUrl($scope.linkTerms);
            $scope.eulaUrl = NavigationFactory.getAbsoluteUrl($scope.model.eulaLink);
            
            $scope.strings.termsText = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'terms-text', {'%pdpUrl%': pdpUrl});
            $scope.strings.trialTimeMessage = UtilFactory.getLocalizedStr(null, CONTEXT_NAMESPACE, 'trialtime', {'%hours%': $scope.model.trialLaunchDuration}, $scope.model.trialLaunchDuration);
            setOthAvailabilityText();
        }

        function setOthAvailabilityText () {
            var useEndDate = $scope.model.useEndDate,
                othAvailabilityText;

            // confirm date is set and is good
            if (useEndDate) {
                othAvailabilityText = UtilFactory.getLocalizedStr($scope.othDateAvailabilityStr, CONTEXT_NAMESPACE, 'oth-date-availability-str', {
                    '%endDate%': moment(useEndDate).format('LL')
                });
            } else {
                othAvailabilityText = UtilFactory.getLocalizedStr($scope.othLimitedAvailabilityStr, CONTEXT_NAMESPACE, 'oth-limited-availability-str');
            }

            $scope.othAvailabilityText = othAvailabilityText;
        }

        $scope.goToEula = function($event) {
            $event.preventDefault();
            NavigationFactory.externalUrl($scope.model.eulaLink, true);
        };

        $scope.goToTerms = function ($event) {
            $event.preventDefault();
            NavigationFactory.openLink($scope.linkTerms);
            $location.hash('terms');
            AppCommFactory.events.fire('navitem:hashChange', 'terms');
        };

        $scope.goToOthPage = function ($event) {
            $event.preventDefault();
            NavigationFactory.openLink(ON_THE_HOUSE_ROUTE);
        };

        $scope.goToPolicyPage = function ($event) {
            $event.preventDefault();
            NavigationFactory.openLink(POLICY_PAGE_ROUTE, {id: $scope.promoLegalId});
        };

        $scope.othUrl = NavigationFactory.getAbsoluteUrl(ON_THE_HOUSE_ROUTE);
        
        $scope.policyUrl = NavigationFactory.getAbsoluteUrl(POLICY_PAGE_ROUTE, {id: $scope.promoLegalId});

        $scope.showOthLink = BooleanEnumeration[$scope.showInfoBubbleOthLink] !== BooleanEnumeration.false;

        $scope.$watchOnce('model', populateScope);
    }

    function originStoreGameLegal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            controller: 'OriginStoreGameLegalCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/game/views/legal.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameLegal
     * @restrict E
     * @element ANY
     * @description
     * @scope
     * @param {LocalizedString} eula-text * The "End User Licence Agreement" text (Merch in defaults only)
     * @param {LocalizedString} terms-text * The "terms and conditions" text (Merch in defaults only)
     * @param {LocalizedString} oth-text * The "About On the House" text (Merch in defaults only)
     * @param {LocalizedString} trialtime * Trial duration messaging (Merch in defaults only)
     * @param {LocalizedString} promo-text * Text for the link to the promo's Terms and Conditions page (Merch in defaults only)
     * @param {LocalizedString} oth-limited-availability-str * On the house availability text. Ex. "Available for a Limited Time Only" (Merch in defaults only)
     * @param {LocalizedString} oth-date-availability-str * TOn the house availability text with end date. Ex "Available Until %endDate%" (Merch in defaults only)
     * @param {BooleanEnumeration} show-oth-link * show oth link or not. Should default to true
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-game-legal></origin-store-game-legal>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameLegalCtrl', OriginStoreGameLegalCtrl)
        .directive('originStoreGameLegal', originStoreGameLegal);
}());
