
/** 
 * @file store/about/scripts/uniquesellingpointmodule.js
 */ 
(function(){
    'use strict';

    function originStoreAboutUniquesellingpointmodule(ComponentsConfigFactory) {
        function originStoreAboutUniquesellingpointmoduleLink (scope, element) {
            var row = element.find('.l-origin-store-row');

            _.forEach(row.children(), function (child) {
                var column = angular.element('<div class="l-origin-store-column-half-stackBelow"></div>');
                column.append(child);
                row.append(column);
            });
        }

        return {
            restrict: 'E',
            scope: {},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/uniquesellingpointmodule.html'),
            link: originStoreAboutUniquesellingpointmoduleLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutUniquesellingpointmodule
     * @restrict E
     * @element ANY
     * @scope
     * @description Container directive for the Unique Selling Point modules on the About page
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-uniquesellingpointmodule />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAboutUniquesellingpointmodule', originStoreAboutUniquesellingpointmodule);
}()); 
