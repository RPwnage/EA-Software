/**
 * @file shell/scripts/miniprofile.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-shell-mini-profile';

    function OriginShellMiniprofileCtrl($scope, $rootScope, UtilFactory) {

        $scope.myNucleusId = Origin.user.userPid;
        $scope.myOriginId = Origin.user.originId;

        $scope.pointsStr = UtilFactory.getLocalizedStr($scope.pointsStr, CONTEXT_NAMESPACE, 'pointscta', {
            '%points%': 0
        });


        $scope.profile = function() {
            location.href = "#/profile";
        };

        $scope.onClick = function() {
            $scope.$emit('shellfooterMenu:clicked');
        };
    }

    function originShellMiniprofile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                'pointsStr': '@pointscta'
            },
            controller: 'OriginShellMiniprofileCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/miniprofile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellMiniprofile
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} pointscta view points action mini profile menu
     * @description
     *
     * mini profile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-miniprofile></origin-shell-miniprofile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginShellMiniprofileCtrl', OriginShellMiniprofileCtrl)
        .directive('originShellMiniprofile', originShellMiniprofile);

}());