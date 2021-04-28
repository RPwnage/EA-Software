/**
 * @file store/about/scripts/section.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */
    var ThemeEnumeration = {
        "light": "light",
        "none": "none"
    };
    /* jshint ignore:end */

    function originStoreAboutSection(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            transclude: true,
            scope: {
                backgroundTheme: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/section.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutSection
     * @restrict E
     * @element ANY
     * @scope
     * @param {ThemeEnumeration} background-theme Theme color of the section. Light means white background. If a value for
     * theme is not present or 'none' is selected, the section takes the body background color.
     *
     * @description about section that does NOT contain a background carousel
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-about-section></origin-store-about-section>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreAboutSection', originStoreAboutSection)
        .directive('originStoreAboutSectionWithbackground', originStoreAboutSection);
}());

