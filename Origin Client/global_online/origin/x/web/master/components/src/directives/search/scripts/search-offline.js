(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-global-search';

    /**
     * Global Search Offline Ctrl
     */
    function OriginSearchOfflineCtrl($scope, UtilFactory) {

        /* Get Translated Strings */
        $scope.strings = {
            offlineTitle: UtilFactory.getLocalizedStr($scope.offlineTitleStr, CONTEXT_NAMESPACE, 'offlinetitle'),
            offlineMessage: UtilFactory.getLocalizedStr($scope.offlineMessageStr, CONTEXT_NAMESPACE, 'offlinemessage'),
            goOnlineButtonLabel:  UtilFactory.getLocalizedStr($scope.goOnlineButtonLabelStr, CONTEXT_NAMESPACE, 'goonlinebuttonlabel')
        };

        $scope.goOnline = function() {
            Origin.client.onlineStatus.goOnline();
        };
    }

    /**
     * Global Search Offline Directive
     */
    function originSearchOffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                offlineTitleStr: '@',
                offlineMessageStr: '@',
                goOnlineButtonLabelStr: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-offline.html'),
            controller: 'OriginSearchOfflineCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSearchOffline
     * @restrict E
     * @scope
     * @param {LocalizedString} offlineTitleStr offline banner title
     * @param {LocalizedString} offlineMessageStr offline banner message
     * @param {LocalizedString} goOnlineButtonLabelStr go online button label text
     * @description
     *
     * Global Search Offline Page
     * When a user is offline (embedded browser) and uses global search, this directive appears after
     * search results (my games library results only when offline) informing the user that they must
     * go online to see more results, and providing a "Go Online" button.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-offline></origin-search-offline>
     *     </file>
     * </example>
     */
    //declaration
    angular.module('origin-components')
        .controller('OriginSearchOfflineCtrl', OriginSearchOfflineCtrl)
        .directive('originSearchOffline', originSearchOffline);
}());