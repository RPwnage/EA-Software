/**
 * @file shell/scripts/loggedin.js
 */
(function() {
    'use strict';

    function OriginShellLoggedinCtrl($scope, AuthFactory) {
        var isLoggedIn = false,
            isUnderAge = false;

        $scope.shouldShow = function() {
            return (isLoggedIn && !isUnderAge);
        };

        function setLoggedInState(loginObj) {
            isLoggedIn = loginObj.isLoggedIn;
            isUnderAge = isLoggedIn && Origin.user.underAge();
        }

        function updateLoggedInState(loginObj) {
            setLoggedInState(loginObj);
            $scope.$digest();
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:change', updateLoggedInState);
        }

        AuthFactory.events.on('myauth:change', updateLoggedInState);

        $scope.$on('$destroy', onDestroy);

        AuthFactory.waitForAuthReady()
            .then(setLoggedInState);
    }

    function originShellLoggedin (ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: true,
            transclude: true,
            controller: 'OriginShellLoggedinCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/checkauth.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellLoggedin
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * shell loggedin(but not underage) panel container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-loggedin></origin-shell-loggedin>
     *     </file>
     * </example>
     */

    angular.module('origin-components')
        .directive('originShellLoggedin', originShellLoggedin)
        .controller('OriginShellLoggedinCtrl', OriginShellLoggedinCtrl);
}());