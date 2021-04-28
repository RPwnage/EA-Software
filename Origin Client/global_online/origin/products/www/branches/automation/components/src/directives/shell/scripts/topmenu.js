/**
 * @file shell/scripts/topmenu.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-shell-top-menu',
        BETA = 'beta',
        ALPHA = 'alpha';

    function OriginShellTopMenuCtrl($scope, NavigationFactory, UtilFactory, ComponentsConfigHelper) {

        $scope.inClient = Origin.client.isEmbeddedBrowser();
        $scope.betaLoc = UtilFactory.getLocalizedStr($scope.beta, CONTEXT_NAMESPACE, 'beta');

        if(ComponentsConfigHelper.getVersion() === BETA) {
            $scope.version = $scope.betaLoc;
        }
        else if(ComponentsConfigHelper.getVersion() === ALPHA) {
            $scope.version = ALPHA;
        }

        function onToggleGlobalSearch(event, toggle) {
            $scope.globalSearchOpen = toggle;
            // stop the event from bubbling up
            event.stopPropagation();
        }

        $scope.$on('globalsearch:toggle', onToggleGlobalSearch);

        $scope.toHome = function ($event) {
            $event.preventDefault();
            NavigationFactory.goHome();
        };
        $scope.homeUrl = NavigationFactory.getAbsoluteUrl('/');

    }

    function originShellTopMenu(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/topmenu.html'),
            controller: 'OriginShellTopMenuCtrl'
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellTopMenu
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} beta "Beta"
     * @description
     *
     * main menu
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-shell-top-menu></origin-shell-top-menu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginShellTopMenuCtrl', OriginShellTopMenuCtrl)
        .directive('originShellTopMenu', originShellTopMenu);
}());
