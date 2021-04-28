(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginTitlebarSubwindowCtrl($scope, $sce) {
        $scope.trustAsHtml = function (html) {
            return $sce.trustAsHtml(html);
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originTitlebarSubwindow
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr the title
     * @description
     *
     * title bar
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-titlebar-subwindow titlestr="Discover Something New!">
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginTitlebarSubwindowCtrl', OriginTitlebarSubwindowCtrl)
        .directive('originTitlebarSubwindow', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                transclude: true,
                controller: 'OriginTitlebarSubwindowCtrl',
                scope: {
                    'titleStr': '@titlestr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('titlebar/views/subwindow.html')
            };

        });
}());

