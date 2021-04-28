/**
 * @file store/pdp/basegamerequired.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-basegamerequired';

    function OriginStorePdpBasegamerequiredCtrl($scope, $state, ProductFactory, UtilFactory, PdpUrlFactory) {
        $scope.model = {};
        $scope.baseGame = {};

        $scope.strings = {
            baseGameRequiredText: UtilFactory.getLocalizedStr($scope.baseGameRequiredText, CONTEXT_NAMESPACE, 'basegamerequiredstr')
        };

        $scope.getBaseGameTitle = function () {
            if ($scope.baseGameTitleText) {
                return $scope.baseGameTitleText;
            } else {
                return $scope.baseGame.displayName;
            }
        };

        $scope.goToBaseGamePdp = function () {
            if ($scope.baseGame) {
                PdpUrlFactory.goToPdp($scope.baseGame);
            }
        };

        /**
         * Checks if the message is visible or not
         * @return {boolean}
         */
        $scope.isVisible = function () {
            return !!$scope.model.isBaseGameRequired;
        };

        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'model');
            }
        });

        $scope.$watch('model.baseGameOfferId', function (newValue) {
            if (newValue) {
                ProductFactory.get(newValue).attachToScope($scope, 'baseGame');
            }
        });
    }

    function originStorePdpBasegamerequired(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                baseGameRequiredText: '@',
                baseGameTitleText: '@',
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/basegamerequired.html'),
            controller: 'OriginStorePdpBasegamerequiredCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpBasegamerequired
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} offer-id product offer ID
     * @param {LocalizedString|OCD} base-game-required-text (optional) 'You Must Have' Text with optional %title% and %link% tokens
     * @param {LocalizedString|OCD} base-game-title-text (optional) Game Title text override
     * @description
     *
     * PDP subscription availability indicator
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-basegamerequired offer-id="DR:156691300"></origin-store-pdp-basegamerequired>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpBasegamerequiredCtrl', OriginStorePdpBasegamerequiredCtrl)
        .directive('originStorePdpBasegamerequired', originStorePdpBasegamerequired);
}());
