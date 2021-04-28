/**
 * @file store/about/scripts/section-content.js
 */

(function () {
    'use strict';

    /**
     * Sets the layout of this module.
     * @readonly
     * @enum {string}
     */
    /* jshint ignore:start */
    var AlignmentEnumeration = {
        "center": "center",
        "left": "left"
    };

    var TextColorEnumeration = {
        "dark": "dark",
        "light": "light"
    };
    /* jshint ignore:end */

    function originStoreAboutSectionContent(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            transclude: true,
            replace: true,
            scope: {
                alignment: '@',
                titleStr: '@',
                subtitle: '@',
                textColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/sectioncontent.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutSectionContent
     * @restrict E
     * @element ANY
     * @scope
     * @param {AlignmentEnumeration} alignment Set heading alignment
     * @param {LocalizedString} title-str Text content for section's title
     * @param {LocalizedString} subtitle Text content for section's subtitle/description
     * @param {TextColorEnumeration} text-color Set color for heading text (defaults to black)
     *
     * @description Wrapper for the heading and content of each about section.
     *              Every section needs this directive. Transcludes the content. Comes
     *              with the heading built in.
     *
     * @example
     * <origin-store-about-section>
     *     <origin-store-about-section-content></origin-store-about-section-content>
     * </origin-store-about-section>
     */
    angular.module('origin-components')
        .directive('originStoreAboutSectionContent', originStoreAboutSectionContent);
}());
