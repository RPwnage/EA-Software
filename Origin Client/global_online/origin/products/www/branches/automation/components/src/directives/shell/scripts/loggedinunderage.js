/**
 * @file shell/scripts/loggedinunderage.js
 */
(function() {
    'use strict';

    function OriginShellLoggedinunderageCtrl($scope, AuthFactory) {
        var isLoggedIn = false,
            isUnderAge = false;

        $scope.shouldShow = function() {
            return (isLoggedIn && isUnderAge);
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

    function originShellLoggedinunderage (ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: true,
            transclude: true,
            controller: 'OriginShellLoggedinunderageCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/checkauth.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellLoggedinunderage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * home panel container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-loggedinunderage></origin-shell-loggedinunderage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originShellLoggedinunderage', originShellLoggedinunderage)
        .controller('OriginShellLoggedinunderageCtrl', OriginShellLoggedinunderageCtrl);
}());