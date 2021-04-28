(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginSocialChatwindowConversationAreaCtrl($scope, UtilFactory) {

	    var CONTEXT_NAMESPACE = 'origin-social-chatwindow-conversation-area';

        $scope.sendAMessageLoc = UtilFactory.getLocalizedStr($scope.sendAMessageStr, CONTEXT_NAMESPACE, 'sendamessagestr');

        var $element, $area, $inputSection, $bannerSection, $historyElement;

        function doPageLayout() {
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
            $historyElement = $element.find('origin-social-chatwindow-conversation-history').children();
        };

        this.inputChanged = function() {
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

        function onClientStateChanged(online) {
            $scope.sendAMessageLoc = online ? UtilFactory.getLocalizedStr($scope.sendAMessageStr, CONTEXT_NAMESPACE, 'sendamessagestr') : '';
            $scope.$digest();
        }

        function onDestroy() {
            Origin.events.off(Origin.events.CLIENT_ONLINESTATECHANGED, onClientStateChanged);
        }

        //listen for connection change (for embedded)
        Origin.events.on(Origin.events.CLIENT_ONLINESTATECHANGED, onClientStateChanged);
        $scope.$on('$destroy', onDestroy);
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
                    $(element).on('changed', 'origin-social-chatwindow-conversation-input', function() {
                        ctrl.inputChanged();
                    });

                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/chatwindow/conversation/views/area.html'),
            };

        });
}());

