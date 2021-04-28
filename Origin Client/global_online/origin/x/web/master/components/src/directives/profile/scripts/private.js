(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfilePrivateCtrl($scope, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-private';

        $scope.privatetitleLoc = UtilFactory.getLocalizedStr($scope.privatetitleStr, CONTEXT_NAMESPACE, 'privatetitlestr');
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfilePrivate
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} privatetitlestr "This user has made their profile private."
     * @description
     *
     * Private Profile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-private
     *            privatetitlestr="This user has made their profile private."
     *         ></origin-profile-private>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfilePrivateCtrl', OriginProfilePrivateCtrl)
        .directive('originProfilePrivate', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfilePrivateCtrl',
                scope: {
                    privatetitleStr: '@privatetitlestr',
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/private.html')
            };

        });
}());

