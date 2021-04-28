
/**
 * @file origin-imagefadein.js
 */
(function(){
    'use strict';

    var SRC_CLASS = 'origin-imagefadein',
        LOADING_CLASS = 'origin-imagefadein-loading';

    function originImagefadein($timeout) {
        function originImagefadeinLink(scope, element) {
            // remove the transition class
            function removeLastClass() {
                element.removeClass(SRC_CLASS);
            }

            // remove the opacity 0 class
            function removeLoadingClass() {
                element.removeClass(LOADING_CLASS);

                // wait till animation is complete
                $timeout(removeLastClass, 300, false);
            }

            element.addClass(SRC_CLASS + ' ' + LOADING_CLASS);
            element.one('load', removeLoadingClass);

            function onDestroy() {
                element.off('load', removeLoadingClass);
            }

            scope.$on('$destroy', onDestroy);
        }

        return {
            restrict: 'A',
            link: originImagefadeinLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originImagefadein
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <img origin-imagefadein ng-src="some image path" />
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originImagefadein', originImagefadein);
}());
