/**
 * @file store/pdp/basegamerequired.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpBasegamerequiredCtrl($scope, UtilFactory, PdpUrlFactory) {
        $scope.goToBaseGamePdp = function ($event) {
            $event.preventDefault();
            if ($scope.baseGameModel) {
                PdpUrlFactory.goToPdp($scope.baseGameModel);
            }
        };
        $scope.$watchOnce('baseGameModel', function(){
            $scope.basegamePdpUrl = PdpUrlFactory.getPdpUrl($scope.baseGameModel, true);
        });



        var stopWatchingOcdDataReady = $scope.$watch('ocdDataReady', function (isOcdDataReady) {
            if (isOcdDataReady) {
                stopWatchingOcdDataReady();
                $scope.baseGameStrings = {
                    baseGameRequiredText: UtilFactory.getLocalizedStr($scope.baseGameRequiredText, CONTEXT_NAMESPACE, 'basegamerequiredstr')
                };
            }
        });
    }

    function originStorePdpBasegamerequired(ComponentsConfigFactory) {
        return {
            restrict: 'E',
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
