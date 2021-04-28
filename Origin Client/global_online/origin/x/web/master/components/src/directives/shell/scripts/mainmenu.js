/**
 * @file shell/scripts/mainmenu.js
 */
(function() {
    'use strict';

    function OriginShellMainMenuCtrl($scope, AppCommFactory) {

        $scope.inClient = Origin.client.isEmbeddedBrowser();
        $scope.sideNavVisible = false;

        /**
         * Display the sidenav when in mobile
         * @return {void}
         * @method
         */
        $scope.toggleSideNav = function(e) {
            if (e) {
                e.preventDefault();
            }
            $scope.sideNavVisible = !$scope.sideNavVisible;
        };

        function onDestroy() {
            AppCommFactory.events.off('menuitem:clicked', $scope.toggleSideNav);
        }

        AppCommFactory.events.on('menuitem:clicked', $scope.toggleSideNav);
        $scope.$on('$destroy', onDestroy);
    }

    function originShellMainMenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/mainmenu.html'),
            controller: 'OriginShellMainMenuCtrl',
            transclude: true
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellMainMenu
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * main menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-shell-main-menu>
     *         <origin-shell-menu-item>
     *             <origin-shell-menu-item-link titlestr="Home" href="#" icon="home"></origin-shell-menu-item-link>
     *         </origin-shell-menu-item>
     *     </origin-shell-main-menu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginShellMainMenuCtrl', OriginShellMainMenuCtrl)
        .directive('originShellMainMenu', originShellMainMenu);
}());