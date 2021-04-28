/**
 * @file store/pdp/scripts/othdetails.js
 */
/* global moment */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-othdetails';

    function OriginStorePdpOthdetailsCtrl($scope, UtilFactory, AppCommFactory) {

        $scope.strings = {
            titleText: UtilFactory.getLocalizedStr($scope.titleText, CONTEXT_NAMESPACE, 'title-text')
        };

        $scope.getAvailabilityText = function() {
            if ($scope.availabilityText) {
                return $scope.availabilityText;
            } else {
                var availabilityDate = $scope.availabilityDate;

                // confirm date is good, if so, display it.
                if (availabilityDate) {
                    return UtilFactory.getLocalizedStr('', CONTEXT_NAMESPACE, 'dateavailabilitystr', {
                        '%endDate%': moment(availabilityDate).format('LL')
                    });
                } else {
                    return UtilFactory.getLocalizedStr($scope.availabilityText, CONTEXT_NAMESPACE, 'availability-text');
                }
            }
        };

        $scope.goToOthPage = function () {
            AppCommFactory.events.fire('uiRouter:go', 'app.store.wrapper.onthehouse');
        };
    }

    function originStorePdpOthdetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                availabilityDate: "=",
                titleText: "@",
                availabilityText: "@"
            },
            controller: "OriginStorePdpOthdetailsCtrl",
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/othdetails.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOthdetails
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP OTH Details block
     * @param {LocalizedString|OCD} title-text OTH Title
     * @param {LocalizedString|OCD} availability-text Availability description
     * @param {LocalizedString} dateavailabilitystr date availability text. Set up once as default.
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-othdetails
     *      title-text="On The House"
     *      availability-text="It will be ready when it's ready!"
     *     ></origin-store-pdp-othdetails>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpOthdetails', originStorePdpOthdetails)
        .controller('OriginStorePdpOthdetailsCtrl', OriginStorePdpOthdetailsCtrl);
}());
