(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-search-offline';

    /**
     * Global Search Offline Ctrl
     */
    function OriginSearchOfflineCtrl($scope, UtilFactory) {

        /* Get Translated Strings */
        $scope.offlineTitleLoc = UtilFactory.getLocalizedStr($scope.offlineTitleStr, CONTEXT_NAMESPACE, 'offlinetitle');
        $scope.offlineMessageLoc = UtilFactory.getLocalizedStr($scope.offlineMessageStr, CONTEXT_NAMESPACE, 'offlinemessage');
        $scope.goOnlineButtonLabelLoc = UtilFactory.getLocalizedStr($scope.goOnlineButtonLabelStr, CONTEXT_NAMESPACE, 'goonlinebuttonlabel');

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
                offlineTitleStr: '@offlinetitle',
                offlineMessageStr: '@offlinemessage',
                goOnlineButtonLabelStr: '@goonlinebuttonlabel'
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
     * @param {LocalizedString} offlinetitle "You're Offline"
     * @param {LocalizedString} offlinemessage "Go online to search the store and people."
     * @param {LocalizedString} goonlinebuttonlabel "Go Online"
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