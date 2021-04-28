(function () {
    'use strict';

    function achievementbadgeFadein() {

        function achievementbadgeFadeinLink(scope, element) {
            element.bind('load', function() {
               element.addClass('origin-achievementbadge-image-fadein');
            });

            scope.$on('$destroy', function() {
                element.unbind('load');
            });
        }

        return {
            restrict: 'A',
            link: achievementbadgeFadeinLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:achievementbadgeFadein
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * Achievement Badge Images Fade In
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <div achievementbadge-fadein>
     *          </div>
     *     </file>
     * </example>
     */
    // declaration
    angular.module('origin-components')
        .directive('achievementbadgeFadein', achievementbadgeFadein);
}());