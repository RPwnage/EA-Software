/**
 * @file shell/scripts/footermenu.js
 */
(function() {
    'use strict';

    function originShellFooterMenuCtrl($scope, AuthFactory, EventsHelperFactory) {

        var handles;

        $scope.ready = false;
        $scope.underAge = Origin.user.underAge();

        function onAuthReady() {
            $scope.ready = true;
        }

        function onDestroy() {
            EventsHelperFactory.detachAll(handles);
        }

        $scope.isUserLogedIn = function () {
            return AuthFactory.isAppLoggedIn();
        };

        //wait for Auth initialization to complete
        AuthFactory.waitForAuthReady()
            .then(onAuthReady);

        handles = [
            AuthFactory.events.on('myauth:ready', $scope.$digest),
            AuthFactory.events.on('myauth:change', $scope.$digest)
        ];
        $scope.$on('$destroy', onDestroy);
    }

    function originShellFooterMenuLoggedout(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/footermenuloggedout.html'),
            controller: originShellFooterMenuCtrl
        };
    }

    function originShellFooterMenuLoggedin(ComponentsConfigFactory, UtilFactory) {

        function originShellFooterMenuLoggedinLink(scope, elem) {

            scope.menuOpen = false;

            function notifyNavigation() {
                scope.$emit('navigation:heightchange');
            }

            function toggleMenuVisibility() {
                scope.menuOpen = !scope.menuOpen;
                var arrow = elem.find('.otkicon-downarrow');
                if (scope.menuOpen) {
                    arrow.addClass('down-arrow-flip');
                } else {
                    scope.$emit('navigation:footermenuclosestart');
                    arrow.removeClass('down-arrow-flip');
                }
                UtilFactory.onTransitionEnd(elem, notifyNavigation);
            }

            scope.$on('miniprofile:toggle', toggleMenuVisibility);
        }

        return {
            restrict: 'E',
            replace: true,
            transclude: true,
            link: originShellFooterMenuLoggedinLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/footermenuloggedin.html'),
            controller: originShellFooterMenuCtrl
        };
    }

    angular.module('origin-components')
        .controller('originShellFooterMenuCtrl', originShellFooterMenuCtrl)
        /**
         * @ngdoc directive
         * @name origin-components.directives:originShellFooterMenuLoggedout
         * @restrict E
         * @element ANY
         * @scope
         * @description
         *
         * footer menulod
         *
         *
         * @example
         * <example module="origin-components">
         *     <file name="index.html">
         *         <origin-shell-footer-menu-loggedout></origin-shell-footer-menu-loggedout>
         *     </file>
         * </example>
         */
        .directive('originShellFooterMenuLoggedout', originShellFooterMenuLoggedout)
        .directive('originShellFooterMenuLoggedin', originShellFooterMenuLoggedin);
}());
