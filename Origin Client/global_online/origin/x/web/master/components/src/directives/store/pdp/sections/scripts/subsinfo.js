/**
 * @file store/pdp/sections/subsinfo.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpSectionsSubsinfoCtrl($scope, ProductFactory, UtilFactory, SubscriptionFactory) {
        $scope.model = {};
        $scope.strings = {
            learnMoreText: UtilFactory.getLocalizedStr($scope.learnMoreText, CONTEXT_NAMESPACE, 'learnmore'),
            availableWithSubsText: UtilFactory.getLocalizedStr($scope.availableWithSubsText, CONTEXT_NAMESPACE, 'availablewithsubs')
        };

        /**
         * Checks whether the Origin Access (Subscription) info block is visible or not
         * @return {boolean}
         */
        $scope.isSubsInfoVisible = function() {
            return $scope.model.isOwned && !SubscriptionFactory.userHasSubscription() && $scope.model.subscriptionAvailable;
        };

        /**
         * Returns localized Origin Access information block
         * @return {string}
         */
        $scope.getSubsInfo = function() {
            return UtilFactory.getLocalizedStr($scope.subsInfoText, CONTEXT_NAMESPACE, 'subsinfo', {
                '%game%': $scope.model.displayName,
                '%subscriptionPrice%': '$9.99'
            });
        };

        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
            }
        });
    }

    function originStorePdpSectionsSubsinfo(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                learnMoreText: '@',
                availableWithSubsText: '@',
                subsInfoText: '@'
            },
            controller: 'OriginStorePdpSectionsSubsinfoCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/subsinfo.html')

        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsSubsinfo
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {string=}
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-sections-subsinfo offer-id="{{ model.offerId }}"></origin-store-pdp-sections-subsinfo>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsSubsinfoCtrl', OriginStorePdpSectionsSubsinfoCtrl)
        .directive('originStorePdpSectionsSubsinfo', originStorePdpSectionsSubsinfo);
}());
