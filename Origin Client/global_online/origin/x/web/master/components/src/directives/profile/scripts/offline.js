(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileOfflineCtrl($scope, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-offline';

        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.descriptionLoc = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');
        $scope.buttonLoc = UtilFactory.getLocalizedStr($scope.buttonStr, CONTEXT_NAMESPACE, 'buttonstr');

        this.goOnline = function () {
            Origin.client.onlineStatus.goOnline();
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileOffline
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr "You're in Offline Mode"
     * @param {LocalizedString} descriptionstr "Profiles are not available when you're offline."
     * @param {LocalizedString} buttonstr "Go Online"
     * @description
     *
     * Profile Offline Page
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-offline
     *            titlestr="You're in Offline Mode"
     *            descriptionstr="Profiles are not available when you're offline."
     *            buttonstr="Go Online"
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
                    titleStr: '@titlestr',
                    descriptionStr: '@descriptionstr',
                    buttonStr: '@buttonstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/offline.html'),
                link: function (scope, element, attrs, ctrl) {
                    scope = scope;
                    attrs = attrs;

                    scope.$element = $(element);

                    // Listen for clicks on the 'Go Online' button
                    $(element).on('click', '.otkbtn', function () {
                        ctrl.goOnline();
                    });
                }
            };

        });
}());

