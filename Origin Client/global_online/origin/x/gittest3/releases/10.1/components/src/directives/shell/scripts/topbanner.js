/**
 * @file shell/scripts/topbanner.js
 */
(function() {
    'use strict';

    function OriginShellTopBannerCtrl($scope, AppCommFactory, UserDataFactory) {

        $scope.isLoaded = false;

        /**
         * Handle login/logout of the app
         * @return {void}
         * @method
         */
        function handleLogin() {
            $scope.isLoaded = true;
        }

        function onDestroy() {
            AppCommFactory.events.off('auth:ready', handleLogin);
        }

        //event may already have been triggered; if so it's saved off in UserDataFactory
        if (UserDataFactory.isAuthReady()) {
            handleLogin();
        } else {
            AppCommFactory.events.on('auth:ready', handleLogin);
            $scope.$on('$destroy', onDestroy);
        }
    }

    function originShellTopBanner(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/topbanner.html'),
            controller: 'OriginShellTopBannerCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellTopBanner
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * top banner
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-top-banner></origin-shell-top-banner>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginShellTopBannerCtrl', OriginShellTopBannerCtrl)
        .directive('originShellTopBanner', originShellTopBanner);

}());