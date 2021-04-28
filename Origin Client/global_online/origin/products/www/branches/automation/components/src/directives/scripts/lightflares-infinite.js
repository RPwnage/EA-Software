/**
 * @file scripts/lightflares-infinite.js
 */
(function(){
    'use strict';

    function originLightflaresInfinite(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/lightflares-infinite.html')
        };
    }
     /**
     * @ngdoc directive
     * @name origin-components.directives:originLightflaresInfinite
     * @restrict E
     * @element ANY
     * @description
     *
     * creates a light flare effect used for gift unveiling (infinite)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-lightflares-infinite>
     *         </origin-lightflares-infinite>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originLightflaresInfinite', originLightflaresInfinite);
}());
