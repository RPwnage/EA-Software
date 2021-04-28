/**
 * @file common/layout-states.js
 */
(function() {
    'use strict';
    // THE MODEL

    function LayoutStatesFactory(ObserverFactory, ObservableFactory, ScreenFactory) {
        var states = ObservableFactory.observe({
            // Defaults set to some properties
            hasFilterOverlay: false,
            mainMenuOpen: false
        });

        var windowWidth = window.innerWidth;

        /**
         * Build an observer for use in the controller
         * @param {Object} scope the current scope
         * @param {string} bindTo the scope variable name to bind to
         * @return {Observer}
         */
        function getObserver() {
            return ObserverFactory
                .create(states);
        }

        /**
         * Set embedded filterpanel state
         * @param {bool} value to be assigned to filterstate
         * @return {void}
         */
        function setEmbeddedFilterpanel (bool) {
            states.data.hasEmbeddedFilter = !!bool;
            states.commit();
        }

        /**
         * Set filter overlay state
         * @param {bool} value to be assigned to overlaystate
         * @return {void}
         */
        function setFilterOverlay (bool) {
            states.data.hasFilterOverlay = !!bool;
            states.commit();
        }

        /**
         * Toggle embedded filterpanel state
         * @return {void}
         */
        function toggleEmbeddedFilterpanel () {
            if(ScreenFactory.isSmall() ) {
                states.data.hasFilterOverlay = !states.data.hasFilterOverlay;
            } else {
                states.data.hasEmbeddedFilter = !states.data.hasEmbeddedFilter;
            }
            states.commit();
        }

        /**
         * Set embedded chat panel state
         * @param {bool} value to be assigned to embedded chat state

         * @return {void}
         */
        function setEmbeddedChat (bool) {
            states.data.hasEmbeddedChat = !!bool;
            states.commit();
        }

        /**
         * Handles a click on the main menu
         *
         * @description
         * @return {void}
         */
         function handleMenuClick () {
             setEmbeddedFilterpanel(false);
             toggleMainMenu();
         }

        /**
         * Toggle main menu state
         *
         * @description At mobile size, toggle overlay state.
         * At desktop size, toggle embedded filter state
         * @return {void}
         */
        function toggleMainMenu () {
            if(ScreenFactory.isSmall()) {
                states.data.mainMenuVisible = !states.data.mainMenuVisible;
            }
            states.commit();
        }

        function closeOverlayIfSmall () {
            if(ScreenFactory.isSmall()) {
                states.data.hasEmbeddedFilter = false;
                states.data.hasFilterOverlay = false;
            }
        }

        /**
         * @description Updates the app after a window resize event
         * Will close the filter after resize at mobile size
         *
         * @return {void}
         */
        function handleWindowResize () {
            // Guards against resize occuring when mobile keyboard
            // appears, modifying Y axis size
            if(windowWidth !== window.innerWidth) {
                closeOverlayIfSmall();
            }

            states.commit();
        }

        return {
            getObserver: getObserver,
            setEmbeddedFilterpanel: setEmbeddedFilterpanel,
            setEmbeddedChat: setEmbeddedChat,
            toggleEmbeddedFilterpanel: toggleEmbeddedFilterpanel,
            toggleMainMenu: toggleMainMenu,
            setFilterOverlay: setFilterOverlay,
            handleMenuClick: handleMenuClick,
            handleWindowResize: handleWindowResize
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.LayoutStatesFactory

     * @description Help determine which panels (if any) are
     * present/overlayed/embedded on page
     *
     * LayoutStatesFactory
     */
    angular.module('origin-components')
        .factory('LayoutStatesFactory', LayoutStatesFactory);
}());
