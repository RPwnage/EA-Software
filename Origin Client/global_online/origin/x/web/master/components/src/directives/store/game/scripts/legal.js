/**
 * @file /store/game/scripts/legal.js
 */
/* global moment */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-game-legal';

    function OriginStoreGameLegalCtrl($scope, ProductFactory, $sce, UtilFactory, PdpUrlFactory, AppCommFactory, $location, NavigationFactory) {

        $scope.strings = {
            eulaText: $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.eulaText, CONTEXT_NAMESPACE, 'eulastr')),
            othText: $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.othText, CONTEXT_NAMESPACE, 'othstr'))
        };

        function setupModel(offerId) {
            if (offerId) {
                ProductFactory.get(offerId).attachToScope($scope, 'model');

                var pdpUrl = PdpUrlFactory.getPdpUrl($scope.model);
                $scope.strings.termsText = UtilFactory.getLocalizedStr($scope.termsText, CONTEXT_NAMESPACE, 'termsstr', {'%pdpUrl%': pdpUrl});

                $scope.linkTerms = PdpUrlFactory.getPdpUrl($scope.model) + '#terms';
            }
        }

        $scope.getOthAvailabilityText = function () {
            var useEndDate = $scope.model.useEndDate;

            // confirm date is set and is good
            if (useEndDate) {
                return UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'othdateavailabilitystr', {
                    '%endDate%': moment(useEndDate).format('LL')
                });
            } else {
                return UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'othlimitedavailabilitystr');
            }
        };

        $scope.goToEula = function() {
            NavigationFactory.externalUrl($scope.model.eulaLink, true);
        };

        $scope.goToTerms = function () {
            $location.hash('terms');
            AppCommFactory.events.fire('navitem:hashChange', 'terms');
        };

        $scope.goToOthPage = function () {
            AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.onthehouse');
        };

        $scope.model = {};
        $scope.$watch('offerId', setupModel);
    }

    function originStoreGameLegal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                offerId: '@offerid',
                eulaText: '@',
                termsText: '@',
                othText: '@'
            },
            controller: 'OriginStoreGameLegalCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('/store/game/views/legal.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreGameLegal
     * @restrict E
     * @element ANY
     * @scope
     * @param {OfferId} offerid the offer id
     * @param {LocalizedString|OCD} eula-text The "End User Licence Agreement" text
     * @param {LocalizedString|OCD} terms-text The "terms and conditions" text
     * @param {LocalizedString|OCD} oth-text The "About On the House" text
     * @description
     *
     * Product details page call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-game-legal
     *              offerid="OFFER-123"
     *              eula-text="End User Licence Agreement"
     *              terms-text="Important terms and conditions apply."
     *              oth-text="About On the House"
     *         ></origin-store-game-legal>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreGameLegalCtrl', OriginStoreGameLegalCtrl)
        .directive('originStoreGameLegal', originStoreGameLegal);
}());
