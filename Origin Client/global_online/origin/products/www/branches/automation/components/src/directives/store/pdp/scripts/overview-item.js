/**
 * @file store/pdp/scripts/overview-item.js
 */
(function(){
    'use strict';

    function originStorePdpOverviewItem(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                message: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/overview-item.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOverviewItem
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} title-str desc
     * @param {LocalizedText} message desc 
     * Single PDP overview block
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-overview-item title-str="Genre" message="{{ model.genre }}"></origin-store-pdp-overview-item>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpOverviewItem', originStorePdpOverviewItem);
}());
