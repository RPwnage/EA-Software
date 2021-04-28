(function() {
    'use strict';

    function SocialHubFactory() {

        var myEvents = new Origin.utils.Communicator(),
            poppedOut = false, minimized = false;

        return {

            events: myEvents,
            
            minimizeWindow: function () {
                minimized = true;
                myEvents.fire('minimizeHub');
            },

            unminimizeWindow: function () {
                minimized = false;
                myEvents.fire('unminimizeHub');
            },
            
            focusWindow: function() {
                if (poppedOut) {
                    myEvents.fire('focusHub');
                }
            },

            popOutWindow: function () {
                poppedOut = true;
                myEvents.fire('popOutHub');
            },

            unpopOutWindow: function () {
                poppedOut = false;
                myEvents.fire('unpopOutHub');
            },

            isWindowMinimized: function () {
                return minimized;
            },

            isWindowPoppedOut: function () {
                return poppedOut;
            }
        };
    }
     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function SocialHubFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('SocialHubFactory', SocialHubFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.ChatWindowFactory
     
     * @description
     *
     * ChatWindowFactory
     */
    angular.module('origin-components')
        .factory('SocialHubFactory', SocialHubFactorySingleton);
}());