/**
 * @file shell/scripts/anonymous.js
 */
(function() {
    'use strict';

    function OriginShellAnonymousCtrl($scope, AuthFactory) {
        var isLoggedIn = false;

        $scope.shouldShow = function() {
            return !isLoggedIn;
        };

        function setLoggedInState(loginobj) {
            isLoggedIn = loginobj.isLoggedIn;
        }

        function updateLoggedInState(loginObj) {
            setLoggedInState(loginObj);
            $scope.$digest();
        }

        AuthFactory.events.on('myauth:change', updateLoggedInState);

        AuthFactory.waitForAuthReady()
            .then(setLoggedInState);
    }


    function originShellAnonymous(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            transclude: true,
            scope: true,
            controller: 'OriginShellAnonymousCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/checkauth.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellAnonymous
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * shell anonymous panel container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-anonymous></origin-shell-anonymous>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellAnonymous', originShellAnonymous)
        .controller('OriginShellAnonymousCtrl', OriginShellAnonymousCtrl);
}());