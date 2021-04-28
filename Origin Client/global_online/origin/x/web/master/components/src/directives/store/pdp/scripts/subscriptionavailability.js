/**
 * @file store/pdp/subscriptionavailability.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-subscriptionavailability';

    function OriginStorePdpSubscriptionavailabilityCtrl($scope, $state, OwnershipFactory, UtilFactory) {

        $scope.strings = {
            viewInLibraryText: UtilFactory.getLocalizedStr($scope.viewInLibraryText, CONTEXT_NAMESPACE, 'viewinlibrary')
        };

        function getTopSubscriptionEdition() {
            return OwnershipFactory.getTopSubscriptionEdition($scope.editions);
        }

        /**
         * Get localized edition-aware ownership message
         * @return {string}
         */
        $scope.getMessage = function () {
            var edition = getTopSubscriptionEdition();

            if (!edition) {
                return '';
            }

            var parameters = {
                '%edition%': edition.editionName
            };

            if (edition.offerId !== $scope.offerId) {
                return UtilFactory.getLocalizedStr($scope.youHaveText, CONTEXT_NAMESPACE, 'youhavethisedition', parameters);
            } else {
                return UtilFactory.getLocalizedStr($scope.youHaveText, CONTEXT_NAMESPACE, 'youhavethis');
            }
        };

        /**
         * Checks if the message is visible or not
         * @return {boolean}
         */
        $scope.isVisible = function () {
            return !!getTopSubscriptionEdition($scope.editions);
        };

        /**
         * Redirects user to the OGD page for the current product
         * @return {void}
         */
        $scope.goToLibrary = function () {
            var edition = getTopSubscriptionEdition($scope.editions);

            $state.go('app.game_gamelibrary.ogd', {
                offerid: edition.offerId
            });
        };
    }

    function originStorePdpSubscriptionavailability(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                editions: '=',
                viewInLibraryText: '@',
                youHaveText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/subscriptionavailability.html'),
            controller: 'OriginStorePdpSubscriptionavailabilityCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSubscriptionavailability
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offer-id product offer ID
     * @param {LocalizedString|OCD} view-in-library-text (optional) link to the library text override
     * @param {LocalizedString|OCD} you-have-text (optional) ownership text override
     * @description
     *
     * PDP subscription availability indicator
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-Subscriptionavailability offer-id="DR:156691300"></origin-store-pdp-Subscriptionavailability>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSubscriptionavailabilityCtrl', OriginStorePdpSubscriptionavailabilityCtrl)
        .directive('originStorePdpSubscriptionavailability', originStorePdpSubscriptionavailability);
}());
