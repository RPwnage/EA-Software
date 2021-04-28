/**
 * @file store/pdp/ownership.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-ownership';

    function OriginStorePdpOwnershipCtrl($scope, $state, OwnershipFactory, UtilFactory) {

        $scope.strings = {
            viewInLibraryText: UtilFactory.getLocalizedStr($scope.viewInLibraryText, CONTEXT_NAMESPACE, 'viewinlibrary')
        };

        /**
         * Get localized edition-aware ownership message
         * @return {string}
         */
        $scope.getMessage = function () {
            var ownedEdition = OwnershipFactory.getTopOwnedEdition($scope.editions);

            if (!ownedEdition) {
                return '';
            }

            var parameters = {
                '%edition%': ownedEdition.editionName
            };

            if (ownedEdition.offerId !== $scope.offerId) {
                return UtilFactory.getLocalizedStr($scope.youOwnText, CONTEXT_NAMESPACE, 'youownthisedition', parameters);
            } else {
                return UtilFactory.getLocalizedStr($scope.youOwnText, CONTEXT_NAMESPACE, 'youownthis');
            }
        };

        /**
         * Checks if the message is visible or not
         * @return {boolean}
         */
        $scope.isVisible = function () {
            return !!OwnershipFactory.getTopOwnedEdition($scope.editions);
        };

        /**
         * Redirects user to the OGD page for the current product
         * @return {void}
         */
        $scope.goToLibrary = function () {
            $state.go('app.game_gamelibrary.ogd', {
                offerid: $scope.offerId
            });
        };
    }

    function originStorePdpOwnership(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                editions: '=',
                viewInLibraryText: '@',
                youOwnText: '@',
                youHaveThroughSubsText: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/ownership.html'),
            controller: 'OriginStorePdpOwnershipCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOwnership
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offer-id product offer ID
     * @param {LocalizedString|OCD} view-in-library-text (optional) link to the library text override
     * @param {LocalizedString|OCD} you-own-text (optional) ownership text override
     * @description
     *
     * PDP product ownership indicator
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-ownership offer-id="DR:156691300"></origin-store-pdp-ownership>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpOwnershipCtrl', OriginStorePdpOwnershipCtrl)
        .directive('originStorePdpOwnership', originStorePdpOwnership);
}());
