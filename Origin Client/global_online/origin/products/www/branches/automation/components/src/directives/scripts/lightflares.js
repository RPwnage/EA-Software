/**
 * @file scripts/lightflares.js
 */
(function(){
    'use strict';

    function originLightflares(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/lightflares.html')
        };
    }
     /**
     * @ngdoc directive
     * @name origin-components.directives:originLightflares
     * @restrict E
     * @element ANY
     * @description
     *
     * creates a light flare effect used for gift unveiling (one-shot)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-lightflares>
     *         </origin-lightflares>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originLightflares', originLightflares);
}());
