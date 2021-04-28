
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
    function originStoreOthDetails(ComponentsConfigFactory, PriceInsertionFactory) {

        function originStoreOthDetailsLink(scope) {
            scope.strings = {};

            PriceInsertionFactory
                .insertPriceIntoLocalizedStr(scope, scope.strings, scope.paragraph, '', 'paragraph');
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                width: '@',
                paragraph: '@',
                titleStr: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/oth/views/details.html'),
            link: originStoreOthDetailsLink
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
     * @param {widthEnumeration} width How wide the component should be displayed
     * @param {LocalizedTemplateString} paragraph the description or details to express
     * @param {LocalizedString} title-str Large title text to introduce the paragraph
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-oth-details width="divided" paragraph="This is a pretty cool little component" title-str="Welcome to Origin"></origin-store-oth-details>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreOthDetails', originStoreOthDetails);
}());
