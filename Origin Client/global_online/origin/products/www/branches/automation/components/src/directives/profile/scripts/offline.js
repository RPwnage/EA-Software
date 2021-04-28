(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileOfflineCtrl($scope, UtilFactory) {
        var CONTEXT_NAMESPACE = 'origin-profile-offline';
        $scope.notAvailableLoc = UtilFactory.getLocalizedStr($scope.notAvailableStr, CONTEXT_NAMESPACE, 'notavailablestr');
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileOffline
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} notavailablestr "Profiles are not available in offline mode."
     * @description
     *
     * Profile Offline Page
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-offline
     *            notavailablestr="Profiles are not available in offline mode."
     *         ></origin-profile-offline>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileOfflineCtrl', OriginProfileOfflineCtrl)
        .directive('originProfileOffline', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfileOfflineCtrl',
                scope: {
                    notAvailableStr: '@notavailablestr',
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/offline.html')
            };

        });
}());

