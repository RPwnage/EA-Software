(function() {
    'use strict';

    function ChatWindowFactory() {

        var myEvents = new Origin.utils.Communicator(),
            poppedOut = false, minimized = false, oigContext = false,
            visibleTabIds = [], overflowTabIds = [];

        return {

            events: myEvents,
            
            openConversation : function(conversationId) {
                myEvents.fire(
                    'openConversation' , conversationId
                );                        
            },

            raiseConversation : function(conversationId) {
                myEvents.fire(
                    'raiseConversation' , conversationId
                );                        
            },

            closeWindow : function (keepConversations) {
                if (typeof keepConversations === 'undefined') {
                    keepConversations = false;
                }
                myEvents.fire('closeChatWindow', keepConversations);
                poppedOut = false;
            },

            minimizeWindow: function () {
                minimized = true;
                myEvents.fire('minimizeChatWindow');                        
            },

            unminimizeWindow: function () {
                minimized = false;
                myEvents.fire('unminimizeChatWindow');                        
            },

            popOutWindow: function () {
                poppedOut = true;
                myEvents.fire('popOutChatWindow');                        
            },

            unpopOutWindow: function () {
                poppedOut = false;
                myEvents.fire('unpopOutChatWindow');                        
            },

            isWindowMinimized: function () {
                return minimized;
            },

            isWindowPoppedOut: function () {
                return poppedOut;
            },

            visibleTabIds: function() {
                return visibleTabIds;
            },

            overflowTabIds: function() {
                return overflowTabIds;
            },
            
            selectTab: function (id) {
                myEvents.fire(
                    'selectTab' , id
                );
            },

            setOIGContext: function (isOIG) {
                oigContext = isOIG;
            },

            isOIGWindow: function() {
                return oigContext;
            }

        };
    }

     /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function ChatWindowFactorySingleton(SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('ChatWindowFactory', ChatWindowFactory, this, arguments);
    }
    /**
     * @ngdoc service
     * @name origin-components.factories.ChatWindowFactory
     
     * @description
     *
     * ChatWindowFactory
     */
    angular.module('origin-components')
        .factory('ChatWindowFactory', ChatWindowFactorySingleton);
}());