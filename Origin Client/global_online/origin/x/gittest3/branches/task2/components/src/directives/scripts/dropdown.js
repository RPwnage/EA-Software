/**
 * Responsible for handling all things dropdown related
 * @file dropdown.js
 */

(function() {
    'use strict';

    function OriginDropdownCtrl($scope) {

        $scope.isVisible = false;

        /**
         * Show / hide the dropdown
         * @return {void}
         * @method toggle
         */
        function toggle() {
            $scope.isVisible = !($scope.isVisible);
            $scope.$digest();
        }

        // PJ: do you have to off on $destroy
        $scope.$on('toggle', toggle);
    }

    function originDropdown(ComponentsConfigFactory, $timeout) {

        function originDropdownLink(scope) {

            // TODO: later this link function will provide focus on the
            // dropdown and allow users to navigate using their keyboard.

            var body = angular.element(document.getElementsByTagName('body'));

            /**
             * When the visibility changes add a click handler
             * to hide the dropdown if the dropdown is visible
             * @return {void}
             * @method onVisibilityChange
             */
            function onVisibilityChange() {
                if (scope.isVisible) {
                    $timeout(function() {
                        body.on('click', hideDropdown);
                    });
                }
            }

            /**
             * Hide the dropdown and then remove the click
             * handler to hide the dropdown
             * @return {void}
             * @method hideDropdown
             */
            function hideDropdown() {
                scope.isVisible = false;
                body.off('click', hideDropdown);
                scope.$digest();
            }

            scope.$watch('isVisible', onVisibilityChange);
        }

        return {
            restrict: 'E',
            scope: {
                dropdownId: '@',
                labeledBy: '@'
            },
            transclude: true,
            link: originDropdownLink,
            controller: 'OriginDropdownCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/dropdown.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDropdown
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} dropdown-id the html id of the drop down
     * @param {string} labeled-by the html id reference for the label
     * @description
     *
     * Wrapper for a dropdown that provides the behaviour of hiding
     * the dropdown when you click outside.  Will also track arrow
     * movements and such
     *
     * @example
     *
     *   <origin-dropdown dropdown-id="miniprofile-dropdown-wrap" labelled-by="miniprofile-dropdown">
     *       <origin-dropdownitem>{{myProfileStr}}</origin-dropdownitem>
     *   </origin-dropdown>
     *
     */
    angular.module('origin-components')
        .controller('OriginDropdownCtrl', OriginDropdownCtrl)
        .directive('originDropdown', originDropdown);

}());