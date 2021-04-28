/**
 * @file store/pdp/scripts/description.js
 */
(function(){
    'use strict';

    function originStorePdpDescriptionCtrl($scope, PdpUrlFactory, ProductFactory, ObjectHelperFactory) {
        var takeHead = ObjectHelperFactory.takeHead;

        ProductFactory
            .filterBy(PdpUrlFactory.getCurrentEditionSelector())
            .addFilter(takeHead)
            .attachToScope($scope, 'model');
    }

    function originStorePdpDescription(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {},
            controller: 'originStorePdpDescriptionCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/description.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpDescription
     * @restrict E
     * @element ANY
     * @scope
     * @description PDP description block, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-description></origin-store-pdp-description>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .controller('originStorePdpDescriptionCtrl', originStorePdpDescriptionCtrl)
        .directive('originStorePdpDescription', originStorePdpDescription);
}());
