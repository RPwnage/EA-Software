(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationAreaCtrl($scope, $timeout, UtilFactory, RosterDataFactory, AuthFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-area';

        $scope.sendAMessageLoc = UtilFactory.getLocalizedStr($scope.sendAMessageStr, CONTEXT_NAMESPACE, 'sendamessagestr');

        var $element, $area, $inputSection, $bannerSection, $historyElement;

        function doPageLayout() {
            // Have to search for these every time or they get "lost" when the window is popped out
            $inputSection = $element.find('.origin-social-conversation-input-section');
            $bannerSection = $element.find('.origin-social-conversation-banner-section');
            $historyElement = $element.find('.origin-social-chatwindow-conversation-history');

            var inputSectionHeight = $inputSection.outerHeight(),
                bannerSectionHeight = $bannerSection.outerHeight();

            $bannerSection.css('bottom', inputSectionHeight + 'px');
            $historyElement.css('bottom', (inputSectionHeight + bannerSectionHeight) + 'px');
        }

        this.setElement = function($el) {
            $element = $el;
            $area = $element.find('.origin-social-conversation-area');
            $inputSection = $element.find('.origin-social-conversation-input-section');
            $bannerSection = $element.find('.origin-social-conversation-banner-section');
            $historyElement = $element.find('.origin-social-chatwindow-conversation-history');
        };

        this.inputResized = function () {
            doPageLayout();
        };

        $scope.getLayoutHeight = function () {
            return $area.innerHeight();
        };

        $scope.$watch($scope.getLayoutHeight, function () {
            // Update our layout when the window size changes
            UtilFactory.throttle(
                function () {
                    doPageLayout();
                }, 100).apply();
        }, true);

        $scope.getBannerHeight = function () {
            return $bannerSection.outerHeight();
        };

        $scope.$watch($scope.getBannerHeight, function () {
            // Update our layout when the banner size changes
            doPageLayout();
        }, true);

        function onClientStateChanged(onlineObj) {
            $scope.sendAMessageLoc = (onlineObj && onlineObj.isOnline) ? UtilFactory.getLocalizedStr($scope.sendAMessageStr, CONTEXT_NAMESPACE, 'sendamessagestr') : '';
            $scope.$digest();
        }

        function onXmppConnect() {
            onClientStateChanged({isLoggedIn: true,
                                  isOnline:true});
        }

        function onXmppDisconnect() {
            onClientStateChanged({isLoggedIn: true,
                                  isOnline:false});
        }

        function onDestroy() {
            AuthFactory.events.off('myauth:clientonlinestatechangeinitiated', onClientStateChanged);
            RosterDataFactory.events.off('xmppConnected', onXmppConnect);
            RosterDataFactory.events.off('xmppDisconnected', onXmppDisconnect);
        }
        $scope.$on('$destroy', onDestroy);

        // listen for connection change (for embedded)
        AuthFactory.events.on('myauth:clientonlinestatechangeinitiated', onClientStateChanged);

        // listen for xmpp disconnect
        RosterDataFactory.events.on('xmppConnected', onXmppConnect);
        RosterDataFactory.events.on('xmppDisconnected', onXmppDisconnect);
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialChatwindowConversationArea
     * @restrict E
     * @element ANY
     * @scope
     * @param {Object} conversation Javascript object representing the conversation
     * @param {LocalizedString} sendamessagestr "Send a message"
     * @description
     *
     * origin chatwindow -> conversation area
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-chatwindow-conversation-area
     *            conversation="conversation"
     *            sendamessagestr="Send a message"
     *         ></origin-social-chatwindow-conversation-area>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialChatwindowConversationAreaCtrl', OriginSocialChatwindowConversationAreaCtrl)
        .directive('originSocialChatwindowConversationArea', function(ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginSocialChatwindowConversationAreaCtrl',
                scope: {
                    conversation: '=',
                    sendAMessageStr: '@sendamessagestr'
                },
                link: function(scope, element, attrs, ctrl) {

                    scope = scope;
                    attrs = attrs;

                    ctrl.setElement($(element));

                    // Listen for changes to the input box so we can resize the history accordingly
                    $(element).on('resized', 'origin-social-chatwindow-conversation-input', function () {
                        ctrl.inputResized();
                    });

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/area.html'),
            };

        });
}());
