/**
 * @file menu/context-menu.js
 */
(function() {
    'use strict';

    var OFFSET = 15,
        CONTEXTMENU_DEFAULT_WIDTH = 180,
        CONTEXTMENUITEM_DEFAULT_HEIGHT = 29,
        CONTEXTMENU_CLASS_SELECTOR = '.otkcontextmenu-wrap',
        CONTEXTMENU_ITEM_SELECTOR = 'li';

    function originContextMenu($compile, $document, ContextMenuFactory) {

        function originContextMenuLink(scope, element) {
            var body = angular.element($document[0].body);

            /**
             * Gets dimensions for contextmenu.
             * If context menu exists in DOM, this function uses those values
             * If context menu does not exist in DOM, fallbacks are provided
             * @param {Object} contextmenu - Jquery reference to contextmenu
             * @return {Object} contextmenuDimensions - Object containing dimensions
             */
             function getContextmenuDimensions (contextmenu) {
                 var contextmenuDimensions = {};

                 if(contextmenu) {
                    contextmenuDimensions.setWidth = contextmenu.length ? contextmenu.width() : CONTEXTMENU_DEFAULT_WIDTH;
                    contextmenuDimensions.setItemHeight = contextmenu.find(CONTEXTMENU_ITEM_SELECTOR).first().height() || CONTEXTMENUITEM_DEFAULT_HEIGHT;
                 }
                 return contextmenuDimensions;

             }

            /**
             * Shows context menu at the current cursor position.
             * Will place contextmenu appropriately to avoid flowing outside window
             * @param {Object} event - Angular event object
             * @return {void}
             */
            function show(event) {
                    // Cursor positions
                var left = event.pageX + OFFSET + 'px',
                    top = event.pageY + OFFSET + 'px',

                    // window dimensions
                    rightBound = window.innerWidth,
                    bottomBound = window.innerHeight,

                    // Vertical scroll position
                    scrollY = document.body.scrollTop,

                    // Distance of cursor from bounds
                    rightOffset = rightBound - (event.pageX + OFFSET),
                    bottomOffset = bottomBound - (event.pageY + OFFSET),

                    // context menu dimensions - default if not populated yet
                    contextmenu = body.find(CONTEXTMENU_CLASS_SELECTOR),

                    contextmenuDimensions = getContextmenuDimensions(contextmenu),

                    // Calculated max space allocated for contextmenu (vert & horiz)
                    rightOffsetMin = contextmenuDimensions.setWidth + OFFSET;

                /**
                 * Determines if context menu is to the right/left
                 * and above/below the cursor
                 * @param {Array} items - items in context menu
                 * @return {Object} coordinates of context menu
                 */
                function getContextmenuPlacement (items) {
                    var contextmenuHeight = items.length * contextmenuDimensions.setItemHeight;

                    // Width checks
                    if(rightOffset < rightOffsetMin) {
                        left = (event.pageX - rightOffsetMin) + 'px';
                    }
                    // Height checks
                    if((bottomOffset + scrollY) < contextmenuHeight) {
                        top = (event.pageY - contextmenuHeight) + 'px';
                    }

                    return { top: top, left: left };
                }

                event.preventDefault();
                event.stopPropagation();

                scope.itemsGenerator().then(function(menuitems) {
                    var contextmenuAlignment = getContextmenuPlacement(menuitems);

                    ContextMenuFactory.open(menuitems, contextmenuAlignment);
                    body.bind('click', hide);
                });
            }

            /**
             * Closes context menu
             * @return {void}
             */
            function hide() {
                ContextMenuFactory.close();

                body.unbind('click', hide);
            }

            // Only hook up the context menu if there are menu items
            scope.itemsGenerator().then(function(menuItems) {
                if (menuItems) {
                    scope.clickEvent = scope.clickEvent || 'contextmenu';
                    element.bind(scope.clickEvent, show);
                }
            });
        }

        return {
            restrict: 'A',
            scope: {
                itemsGenerator: '&originContextMenu',
                clickEvent: '@?'
            },
            link: originContextMenuLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originContextMenu
     * @restrict A
     * @element ANY
     * @param {object|function} origin-context-menu model object or factory method producing
     *        array of context menu items of following format:
     *                  {
     *                      label: 'some text to show in the menu'
     *                      callback: function() {...item click handler...}
     *                      enabled: true|false
     *                  }
     * @scope
     * @description
     *
     * Adds right-click context menu to DOM element
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <div class='origin-gametile' origin-context-menu='getAvailableActions()'></div>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .directive('originContextMenu', originContextMenu);

}());
