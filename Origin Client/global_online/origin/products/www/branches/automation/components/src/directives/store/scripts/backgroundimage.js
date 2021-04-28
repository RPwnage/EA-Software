/**
 * @file store/scripts/backgroundimage.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function originStoreBackgroundimage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                backgroundColor: '@',
                image: '@',
                bottomScrim: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/backgroundimage.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBackgroundimage
     * @restrict E
     * @element ANY
     *
     * @param {string} background-color The background color to fade to
     * @param {ImageUrl} image The image to use as the background image
     * @param {BooleanEnumeration} bottom-scrim should this background image have a bottom scrim
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-backgroundimage background-color='{{ ::backgroundColor }}' image='{{ ::imageSrc }}'> </origin-store-backgroundimage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreBackgroundimage', originStoreBackgroundimage);
}());
