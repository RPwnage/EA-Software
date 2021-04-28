/**
 * @file common/context-menu.js
 */
(function() {
    'use strict';

    function ContextMenuFactory($document, $rootScope, $compile) {
        var element, scope;

        /**
         * Create the menu and add it to the body
         * @return {void}
         */
        function ensureMenuCreated() {
            var body = angular.element($document[0].body);

            if (!element) {
                scope = $rootScope.$new();
                scope.isVisible = true;
                scope.items = [];

                element = $compile('<origin-dropdown-menu items="items" top="top" left="left" is-visible="isVisible"></origin-dropdown-menu>')(scope);
                body.append(element);
            }
        }

        /**
         * Open the menu
         * @param {Array} items - menu items
         * @param {Object} position - menu location on screen
         * @return {void}
         */
        function open(items, position) {
            ensureMenuCreated();

            scope.items = items;
            scope.left = position.left;
            scope.top = position.top;
            scope.isVisible = true;
            scope.$digest();
        }


        /**
         * Close the menu
         * @return {void}
         */
        function close() {
            ensureMenuCreated();
            scope.isVisible = false;
            scope.$digest();
        }

        return {
            open: open,
            close: close
        };
    }


    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ContextMenuFactorySingleton($document, $rootScope, $compile, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ContextMenuFactory', ContextMenuFactory, this, arguments);
    }


    /**
     * @ngdoc service
     * @name origin-components.factories.ContextMenuFactory

     * @description
     *
     * ContextMenuFactory
     */
    angular.module('origin-components')
        .factory('ContextMenuFactory', ContextMenuFactorySingleton);

}());