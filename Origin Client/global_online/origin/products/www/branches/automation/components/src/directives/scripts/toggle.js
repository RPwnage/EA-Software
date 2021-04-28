/**
 * Eyar, he be toggle.  Toggle is responsible for toggling classes. It can be used
 * to show / hide dropdowns or other fancy spancy things.
 * @file toggle.js
 */

(function() {
    'use strict';

    function originToggle() {

        function originToggleLink(scope, element, attrs) {

            var targ,
                toggleClass,
                toggleByEvent;

            /**
            * Store the variables the first time to click
            * on the toggle element
            * @return {void}
            * @method initToggleVars
            */
            function initToggleVars() {
                if (typeof(targ) === 'undefined') {
                    targ = angular.element(document.querySelectorAll(attrs.originToggle));
                    toggleClass = attrs.originToggleClass;
                    toggleByEvent = (typeof(attrs.originToggleEvent) !== 'undefined');
                }
            }

            /**
            * Toggle the toggleClass on the element
            * @return {void}
            * @method toggleClasses
            */
            function toggleClasses() {
                if (targ.hasClass(toggleClass)) {
                    targ.removeClass(toggleClass);
                } else {
                    targ.addClass(toggleClass);
                }
            }

     

            /**
            * Toggle an event or the class name on teh target
            * @return {void}
            * @method toggleTarget
            */
            function toggleTarget() {
                initToggleVars();
                if (!toggleByEvent)  {
                    toggleClasses();
                }
            }

            /**
            * Cleanup references and listeners
            * @return {void}
            * @method mrSelfDestruct
            */
            function mrSelfDestruct() {
                targ =
                toggleClass = null;
                element.off('click', toggleTarget);
            }

            if (attrs.originToggle) {
                element.on('click', toggleTarget);
                scope.$on('$destroy', mrSelfDestruct);
            }

        }

        return {
            restrict: 'A',
            link: originToggleLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originToggle
     * @restrict A
     * @element ANY
     * @scope
     * @param {string} origin-toggle any query selector eg. #id41
     * @param {string} origin-toggle-class the classname to apply to the matches eg .invisible
     * @description
     *
     * Toggle a class on an element on click
     *
     * @example
     * <a class="otkdropdown-trigger origin-miniprofile-dropdown-trigger otkdropdown-trigger-caret otktitle-4 otktitle-4-strong" href="javascript:void(0);" origin-toggle="#miniprofile-dropdown-wrap" origin-toggle-class="otkdropdown-isvisible">{{myOriginId}}</a>
     * <div class="otkdropdown-wrap" id="miniprofile-dropdown-wrap">
     *     <ul class="otkmenu otkmenu-dropdown" role="menu" arialabelledby="miniprofileDropdown">
     *         <li role="presentation"><a role="menuitem" tabindex="-1" href="javascript:void(0)">{{myProfileStr}}</a></li>
     *         <li role="presentation"><a role="menuitem" tabindex="-1" href="javascript:void(0)">{{accountStr}}</a></li>
     *         <li role="presentation"><a role="menuitem" tabindex="-1" href="javascript:void(0)">{{orderStr}}</a></li>
     *         <li role="presentation"><a role="menuitem" tabindex="-1" href="javascript:void(0)" ng-click="logout()">{{logoutStr}}</a></li>
     *     </ul>
     * </div>
     *
     */
    angular.module('origin-components')
        .directive('originToggle', originToggle);

}());