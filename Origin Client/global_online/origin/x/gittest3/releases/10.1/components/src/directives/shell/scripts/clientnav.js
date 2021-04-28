/**
 * @file shell/scripts/clientnav.js
 */
(function() {
    'use strict';

    /*
     * Controller Client Nav Ctrl
     */
    function OriginShellClientNavCtrl($scope, $window) {
        // need to keep track of how often we move backwards
        var backCount = 0,
            forwardCount = -1, // -1 as when the client loads it will setButtons
            userInitiated = false; // we need to know if the user is toggling the change

        /**
         * Determine if we can move backwards. We are relying
         * on the fact that the user isn't going to refresh
         * the client for these numbers to work properly.
         * @return {Boolean}
         */
        function canMoveBack() {
            return forwardCount > 0;
        }

        /**
         * Determine if we can move forwards
         * @return {Boolean}
         */
        function canMoveForward() {
            return backCount > 0;
        }

        /**
         * Disable/enable the buttons
         * @return {void}
         */
        function setButtons() {
            if (!userInitiated) {
                forwardCount++;
            } else {
                userInitiated = false;
            }
            $scope.backDisabled = !canMoveBack();
            $scope.forwardDisabled = !canMoveForward();
        }

        /**
         * Navigate backwards
         * @return {void}
         */
        $scope.goBack = function() {
            if (canMoveBack()) {
                $window.history.back();
                backCount++;
                forwardCount--;
                userInitiated = true;
            }
        };

        /**
         * Navigate forwards
         * @return {void}
         */
        $scope.goForwards = function() {
            if (canMoveForward()) {
                $window.history.forward();
                backCount--;
                forwardCount++;
                userInitiated = true;
            }
        };

        /**
         * Constructor
         * @return {void}
         */
        this.init = function() {
            setButtons();
            //PJ: do we need to $off this?
            $scope.$on('$locationChangeSuccess', setButtons);
        };
    }

    /*
     * Client Nav Search Directive
     */
    function originShellClientNav(ComponentsConfigFactory) {

        function originShellClientNavLink(scope, elem, attrs, ctrl) {
            ctrl.init();
        }

        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/clientnav.html'),
            controller: 'OriginShellClientNavCtrl',
            scope: true,
            link: originShellClientNavLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellClientNav
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Client Navigation
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-client-nav></origin-shell-client-nav>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginShellClientNavCtrl', OriginShellClientNavCtrl)
        .directive('originShellClientNav', originShellClientNav);

}());