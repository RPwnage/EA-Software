
/**
 * @file store/scripts/informational.js
 */
(function(){
    'use strict';

    function originStoreInformational(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                infoText: '@',
                href: '@',
                description: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/informational.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreInformational
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} info-text The Text for the module
     * @param {LocalizedString} description The text for the call to action
     * @param {Url} href The url to link this module to.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-informational info-text="This is the main text of the component" href="/store"/ description='Click Me'>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreInformational', originStoreInformational);
}());
