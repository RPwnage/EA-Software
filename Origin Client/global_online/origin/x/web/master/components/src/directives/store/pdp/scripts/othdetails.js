/**
 * @file store/pdp/scripts/othdetails.js
 */
/* global moment */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-othdetails';

    function OriginStorePdpOthdetailsCtrl($scope, UtilFactory, $sce, AppCommFactory) {

        $scope.strings = {
            titleText: UtilFactory.getLocalizedStr($scope.titleText, CONTEXT_NAMESPACE, 'titlestr'),
            learnMoreText: $sce.trustAsHtml(UtilFactory.getLocalizedStr($scope.learnMoreText, CONTEXT_NAMESPACE, 'learnmorestr'))
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
                    return UtilFactory.getLocalizedStr($scope.availabilityText, CONTEXT_NAMESPACE, 'limitedavailabilitystr');
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
                availabilityText: "@",
                learnMoreText: "@"
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
     * @param {LocalizedString|OCD} availability-text Availability decsription
     * @param {LocalizedString|OCD} learn-more-text Learn More Link text. includes link
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-othdetails
     *      title-text="On The House"
     *      availability-text="It will be ready when it's ready!"
     *      learn-more-text="Learn More"
     *     ></origin-store-pdp-othdetails>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpOthdetails', originStorePdpOthdetails)
        .controller('OriginStorePdpOthdetailsCtrl', OriginStorePdpOthdetailsCtrl);
}());
