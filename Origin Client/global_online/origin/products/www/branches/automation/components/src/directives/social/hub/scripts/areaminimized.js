/**
 * @file areaminimized.js
 */
(function() {

    'use strict';

    /**
     * The controller
     */
    function OriginSocialHubAreaminimizedCtrl($scope, $timeout, ChatWindowFactory, ConversationFactory, RosterDataFactory, SocialHubFactory, AuthFactory, ComponentsLogFactory, SocialDataFactory, EventsHelperFactory, UtilFactory, ObserverFactory) {

        var CONTEXT_NAMESPACE = 'origin-social-hub-areaminimized';

        $scope.chatLoc = UtilFactory.getLocalizedStr($scope.chatStr, CONTEXT_NAMESPACE, 'chatstr');
        $scope.offlineLoc = UtilFactory.getLocalizedStr($scope.offlineStr, CONTEXT_NAMESPACE, 'offlinestr');

        var $element,
        handles = [];

        RosterDataFactory.getRosterUser(Origin.user.userPid()).then(function(user) {
            if (!!user) {
                ObserverFactory.create(user._presence)
                    .defaultTo({jid: Origin.user.userPid(), presenceType: '', show: 'UNAVAILABLE', gameActivity: {}})
                    .attachToScope($scope, 'presence');
            } 
        });         

        ObserverFactory.create(RosterDataFactory.getOnlineCountWatch())
            .attachToScope($scope, 'onlineCount');
        

        // listen for xmpp connect/disconnect
        ObserverFactory.create(RosterDataFactory.getXmppConnectionWatch())
            .defaultTo({isConnected: Origin.xmpp.isConnected()})
            .attachToScope($scope, 'xmppConnection');       


        $scope.isPoppedOut = function() {
            return SocialHubFactory.isWindowPoppedOut();
        };

        this.setElement = function($el) {
            $element = $el;
        };

        function onUnminimizeHub() {
            $element.find('.origin-social-hub-area-minimized').css('max-height', '0');
        }

        function onMinimizeHub() {
            $timeout(function() {
                $element.find('.origin-social-hub-area-minimized').css('max-height', '100%');

            }, 400);
        }

        handles[handles.length] = SocialHubFactory.events.on('unminimizeHub', onUnminimizeHub);
        handles[handles.length] = SocialHubFactory.events.on('minimizeHub', onMinimizeHub);

        function destroy() {
            EventsHelperFactory.detachAll(handles);
        }

        window.addEventListener('unload', destroy);

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubAreaminimized
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} chatstr "Chat (%count%)"
     * @param {LocalizedString} offlinestr "Chat Offline"
     * @description
     *
     * origin social hub -> area minimized
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-areaminimized
     *            chatstr="Chat (%count%)"     
     *            offlinestr="Chat Offline"     
     *         ></origin-social-hub-areaminimized>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubAreaminimizedCtrl', OriginSocialHubAreaminimizedCtrl)
        .directive('originSocialHubAreaminimized', function(ComponentsConfigFactory, SocialHubFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialHubAreaminimizedCtrl',
                scope: {
                    chatStr: '@chatstr',              
                    offlineStr: '@offlinestr'                
                },                
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/areaminimized.html'),
                link: function(scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    ctrl.setElement($(element));

                    $(element).find('.origin-social-hub-area-minimized').on('click', function() {
                        SocialHubFactory.unminimizeWindow();
                    });

                }
            };

        });

}());
