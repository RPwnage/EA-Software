/**
 * @file store/pdp/sections/offerdetails.js
 */
(function(){
    'use strict';

    /* global moment */
    var CONTEXT_NAMESPACE = 'origin-store-pdp-hero';

    function OriginStorePdpSectionsOfferdetailsCtrl($scope, ProductFactory, UtilFactory, DateHelperFactory) {
        $scope.model = {};

        /**
         * Returns true if the game hasn't been released yet
         * @return {boolean}
         */
        $scope.isNotReleased = function () {
            return DateHelperFactory.isInTheFuture($scope.model.releaseDate);
        };

        /**
         * Returns formatted and localized preload date message
         * @return {string}
         */
        $scope.getPreloadDate = function () {
            if (DateHelperFactory.isInThePast($scope.model.downloadStartDate)) {
                return UtilFactory.getLocalizedStr($scope.nowAvailableText, CONTEXT_NAMESPACE, 'preloadavailable');
            } else {
                return UtilFactory.getLocalizedStr($scope.preloadDateText, CONTEXT_NAMESPACE, 'preloaddate', {
                    '%preloaddate%': moment($scope.model.downloadStartDate).format('LL')
                });
            }
        };

        /**
         * Returns formatted and localized release date message
         * @return {string}
         */
        $scope.getReleaseDate = function() {
            return UtilFactory.getLocalizedStr($scope.releaseDateText, CONTEXT_NAMESPACE, 'releasedate', {
                '%releasedate%': moment($scope.model.releaseDate).format('LL')
            });
        };

        $scope.$watch('offerId', function (newValue) {
            if (newValue) {
                ProductFactory.get($scope.offerId).attachToScope($scope, 'model');
            }
        });
    }

    function originStorePdpSectionsOfferdetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offerId: '@',
                nowAvailableText: '@',
                preloadDateText: '@',
                releaseDateText: '@'
            },
            controller: 'OriginStorePdpSectionsOfferdetailsCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/sections/views/offerdetails.html')

        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpSectionsOfferdetails
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
     *         <origin-store-pdp-sections-offerdetails offer-id="{{ model.offerId }}"></origin-store-pdp-sections-offerdetails>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStorePdpSectionsOfferdetailsCtrl', OriginStorePdpSectionsOfferdetailsCtrl)
        .directive('originStorePdpSectionsOfferdetails', originStorePdpSectionsOfferdetails);
}());
