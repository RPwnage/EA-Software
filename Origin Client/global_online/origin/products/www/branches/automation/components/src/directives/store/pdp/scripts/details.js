/**
 * @file store/pdp/scripts/details.js
 */
(function(){
    'use strict';

    function originStorePdpDetailsCtrl($scope) {

        this.registerItemWithData = function() {
            $scope.detailsItemAdded();
        };
    }

    function originStorePdpDetails(ComponentsConfigFactory) {

        function originStorePdpDetailsLink(scope, element, attributes, originStorePdpSectionWrapperCtrl) {
            scope.detailsItemAdded = function() {
                originStorePdpSectionWrapperCtrl.setVisibility(true);
            };
        }

        return {
            restrict: 'E',
            require: '^originStorePdpSectionWrapper',
            scope: {},
            transclude: true,
            link: originStorePdpDetailsLink,
            controller: originStorePdpDetailsCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/details.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDetails
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP details blocks,retrieved from cq5
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-details></origin-store-pdp-details>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpDetails', originStorePdpDetails);
}());
