(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileAnonymousCtrl($scope, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-anonymous';

        $scope.titleLoc = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'titlestr');
        $scope.descriptionLoc = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'descriptionstr');

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileAnonymous
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} titlestr "NOT LOGGED IN"
     * @param {LocalizedString} descriptionstr "You are currently not logged in. Please log in to access the full range of Origin features."
     * @description
     *
     * Profile Anonymous Page
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-anonymous
     *            titlestr="NOT LOGGED IN"
     *            descriptionstr="You are currently not logged in. Please log in to access the full range of Origin features."
     *         ></origin-profile-anonymous>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileAnonymousCtrl', OriginProfileAnonymousCtrl)
        .directive('originProfileAnonymous', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfileAnonymousCtrl',
                scope: {
                    titleStr: '@titlestr',
                    descriptionStr: '@descriptionstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/anonymous.html')
            };

        });
}());

