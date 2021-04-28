/**
 * @file shell/scripts/footermenu.js
 */
(function() {
    'use strict';

    function originShellFooterMenuCtrl($scope, AuthFactory) {
        $scope.isUserLogedIn = function () {
            return AuthFactory.isAppLoggedIn();
        };

        $scope.ready = false;

        $scope.updateUI = function() {
            $scope.$digest();
        };

        function onDestroy() {
            AuthFactory.events.off('myauth:ready', $scope.updateUI);
            AuthFactory.events.off('myauth:change', $scope.updateUI);
        }

        AuthFactory.events.on('myauth:ready', $scope.updateUI);
        AuthFactory.events.on('myauth:change', $scope.updateUI);
        $scope.$on('$destroy', onDestroy);


       //wait for Auth initialization to complete
        AuthFactory.waitForAuthReady()
            .then(function() {
                $scope.ready = true;
        });
    }

    function originShellFooterMenuLoggedout(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/footermenuloggedout.html'),
            transclude: true,
            controller: originShellFooterMenuCtrl
        };
    }

    function originShellFooterMenuLoggedin(ComponentsConfigFactory, UtilFactory) {

        function originShellFooterMenuLoggedinLink(scope, elem) {
            scope.menuOpen = false;

            function toggleMenuVisibility() {
                scope.menuOpen = !scope.menuOpen;
                var menuWrapper, arrow;
                if (scope.menuOpen) {
                    var menuElement = elem.find('.l-origin-bottomnav');
                    var height = UtilFactory.getElementHeight(menuElement);
                    menuWrapper = elem.find('.origin-shell-bottom-wrapper');
                    menuWrapper.height(height);
                    arrow = elem.find('.otkicon-downarrow');
                    arrow.addClass('down-arrow-flip');
                } else {
                    menuWrapper = elem.find('.origin-shell-bottom-wrapper');
                    var miniProfile = elem.find('.origin-miniprofile');
                    arrow = elem.find('.otkicon-downarrow');
                    arrow.removeClass('down-arrow-flip');

                    // Get the height of the mini profile and the padding of it's container
                    var miniProfileHeight = UtilFactory.getElementHeight(miniProfile) + parseInt(elem.find('.l-origin-bottomnav').css('padding-top'), 10);
                    menuWrapper.height(miniProfileHeight);
                }
            }

            scope.$on('shellfooterMenu:clicked', toggleMenuVisibility);
    }

        return {
            link: originShellFooterMenuLoggedinLink,
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/footermenuloggedin.html'),
            transclude: true,
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