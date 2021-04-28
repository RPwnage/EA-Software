/**
 * @file store/about/scripts/heading.js
 */

(function(){
    'use strict';

    /* jshint ignore:start */
    var TextColorEnumeration = {
        "dark": "dark",
        "light": "light"
    };

    var AlignmentEnumeration = {
        "center": "center",
        "left": "left"
    };
    /* jshint ignore:end */

    function originStoreAboutHeading(ComponentsConfigFactory) {

        return {
            restrict: 'E',
            scope: {
                alignment: '@',
                titleStr: '@',
                subtitle: '@',
                textColor: '@'
            },
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/heading.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutHeading
     * @restrict E
     * @element ANY
     * @scope
     * @param {AlignmentEnumeration} alignment Set heading alignment
     * @param {String} title-str Text content for section's title
     * @param {String} subtitle Text content for section's subtitle/description
     * @param {TextColorEnumeration} text-color Set color for heading text (defaults to black)
     * @description
     *
     * About Module heading/description
     *
     * @example
     * <origin-store-about-heading
     *  alignment="center"
     *  title-str="Play your way"
     *  subtitle="Buy, subscribe, or play for free. Awesome games, however you want 'em."
     *  text-color="light">
     * </origin-store-about-heading>
     */
    angular.module('origin-components')
        .directive('originStoreAboutHeading', originStoreAboutHeading);
}());