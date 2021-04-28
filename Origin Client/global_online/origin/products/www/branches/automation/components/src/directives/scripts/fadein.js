/**
 * @file scripts/fadein.js
 */
(function () {
    'use strict';

    /* Directive declaration*/
    function originFadein(ScreenFactory, AnimateFactory) {

        /* Directive Link */
        function originFadeinLink(scope, element) {
            element.addClass('origin-fadein');

            function updateVisibility() {
                element.toggleClass('origin-fadein-visible', ScreenFactory.isOnOrAboveScreen(element));
            }

            AnimateFactory.addScrollEventHandler(scope, updateVisibility);


            setTimeout(updateVisibility, 20); //need to run the very first time (in case there is no scroll bar)
        }

        return {
            restrict: 'A',
            link: originFadeinLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originFadein
     * @restrict A
     * @element ANY
     * @scope
     *
     * @description Causes all nested elements (direct descendant) to fade in and slide upwards,
     *              in a staggered manner.  The child elements MUST be direct descendants -
     *              use "replace: true;" in your directive if necessary.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <div origin-fadein>
     *             <div>Lorem Ipsum</div>
     *             <div>Lorem Ipsum Dolor</div>
     *             <div>Lorem Ipsum Dolor Sit Amet</div>
     *         </div>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originFadein', originFadein);
}());
