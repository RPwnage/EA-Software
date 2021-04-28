
/**
 * @file store/oth/scripts/details.js
 */
(function(){
    'use strict';
    /* jshint ignore:start */
    var widthEnumeration = {
        "full": "full",
        "half": "half",
        "third": "third",
        "quarter": "quarter"
    };
    /* jshint ignore:end */

    /**
    * The directive
    */
    function originStoreOthDetails(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                width: '@',
                paragraph: '@',
                title: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/oth/views/details.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreOthDetails
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {width} widthEnumeration How wide the component should be displayed
     * @param {LocalizedString} paragraph the description or details to express
     * @param {LocalizedString} title Large title text to introduce the paragraph
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-oth-details width="divided" paragraph="This is a pretty cool little component" title="Welcome to Origin"></origin-store-oth-details>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreOthDetails', originStoreOthDetails);
}());
