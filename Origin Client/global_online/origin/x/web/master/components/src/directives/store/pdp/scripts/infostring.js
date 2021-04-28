/**
 * @file store/pdp/infostring.js
 */
(function(){
    'use strict';

    function originStorePdpInfostring(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                isVisible: '&',
                message: '&'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/infostring.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpInfostring
     * @restrict E
     * @element ANY
     * @scope
     * @param {boolean} is-visible directive visibility flag
     * @param {string} message text message
     * @description
     *
     * Important one-line information
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-infostring message="Now Available" is-visible="isGameAvailable()"></origin-store-pdp-infostring>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpInfostring', originStorePdpInfostring);
}());
