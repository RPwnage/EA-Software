/**
 * @file store/pdp/contextmenu.js
 */
(function () {
    'use strict';

    function originStoreContextmenu(ComponentsConfigFactory, ScreenFactory) {
        function originStoreContextmenuLink(scope, element) {
            var CONTEXT_MENU_MARGIN = 4,
                BTN_AND_CONTEXT_MENU_AREA_SELECTOR = '.origin-store-contextmenu-wrapper',
                BODY_SELECTOR = '.origin-content';

            scope.isContextMenuVisible = false;

            function updateDropdownPosition() {
                var windowHeight = angular.element(window).height(),
                    buttonHeight = element.height(),
                    menuBottom = element.offset().top - angular.element(document).scrollTop() + buttonHeight,
                    contextMenuHeight = element.find('.otkcontextmenu-wrap').height();

                var newPosition = buttonHeight + CONTEXT_MENU_MARGIN;

                if ( ScreenFactory.isSmall() || (menuBottom + contextMenuHeight) > windowHeight ) {
                    scope.top = 'auto';
                    scope.bottom = newPosition;
                } else {
                    scope.top = newPosition;
                    delete scope.bottom;
                }

            }

            scope.toggleContextMenu = function () {
                scope.isContextMenuVisible = !scope.isContextMenuVisible;

                // Emit an event to remove any info bubble that may be showing
                if (scope.isContextMenuVisible) {
                    scope.$emit('infobubble-remove');
                }

                updateDropdownPosition();
            };

            scope.hideContextMenu = function () {
                scope.isContextMenuVisible = false;
            };

            // Dismisses the contextmenu on a click outside the dropdown and cmenu itself
            function handleBodyClick (e) {
                var clickableArea = angular.element(BTN_AND_CONTEXT_MENU_AREA_SELECTOR);

                if(scope.isContextMenuVisible && !clickableArea.has(e.target).length) {
                    scope.hideContextMenu();
                    scope.$digest();
                }
            }

            angular.element(BODY_SELECTOR).on('click', handleBodyClick);


            /**
             * Unhook from events when directive is destroyed.
             * @method onDestroy
             */
            function onDestroy() {
                angular.element(BODY_SELECTOR).off('click', handleBodyClick);
            }

            scope.$on('$destroy', onDestroy);

        }

        return {
            restrict: 'E',
            scope: {
                items: '=',
                type: '@'
            },
            link: originStoreContextmenuLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/contextmenu.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreContextmenu
     * @restrict E
     * @element ANY
     * @scope
     * @description ContextMenu for store
     * @param {string} type desc
     * @param {Array} items list of items
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *          <origin-store-contextmenu type="transparent" items="contextMenuItems" ng-if="isGiftable"></origin-store-contextmenu>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreContextmenu', originStoreContextmenu);
}());
