/**
 * @file fallbackimage.js
 */
(function() {
    'use strict';

    function originFallbackimage() {

        function setDefaultPackArt(elem, defaultPackArt) {
            elem.attr('src', defaultPackArt);
        }

        function originFallbackimageLink(scope, elem, attrs) {
            elem.bind('error', function() {
                setDefaultPackArt(elem, attrs.originFallbackimage);
            });

            scope.$on('$destroy', function() {
                elem.unbind('error');
            });
        }


        return {
            restrict: 'A',
            link: originFallbackimageLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFallbackimage
     * @restrict A
     * @scope
     * @param {string} default-pack-art the url for the default box art
     * @description
     *
     * fallback boxart image
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <img origin-fallbackimage='{{defaultPackArt}}' ng-src='{{packArt}}' alt='{{displayName}}'>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originFallbackimage', originFallbackimage);
}());