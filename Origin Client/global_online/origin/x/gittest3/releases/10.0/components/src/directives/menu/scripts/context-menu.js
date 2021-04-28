/**
 * @file menu/context-menu.js
 */
(function() {
    'use strict';

    function originContextMenu($compile, $document, ContextMenuFactory) {
        var OFFSET = 15;

        function originContextMenuLink(scope, element) {
            var body = angular.element($document[0].body);

            /**
             * Shows context menu at the current cursor position
             * @param {Object} event - Angular event object
             * @return {void}
             */
            function show(event) {
                var left = event.pageX + OFFSET + 'px',
                    top = event.pageY + OFFSET + 'px',
                    items;

                event.preventDefault();
                event.stopPropagation();

                scope.itemsGenerator().then(function(menuitems) {
                    items = menuitems;
                    ContextMenuFactory.open(items, {top: top, left: left});
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

            scope.clickEvent = scope.clickEvent || 'contextmenu';
            element.bind(scope.clickEvent, show);
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
     * @param {object|function} originContextMenu model object or factory method producing
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