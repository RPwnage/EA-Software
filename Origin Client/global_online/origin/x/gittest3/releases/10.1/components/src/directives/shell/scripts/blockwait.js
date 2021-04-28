/**
 * @file shell/scripts/blockwait.js
 */
(function() {
    'use strict';

    function OriginShellBlockwaitCtrl($scope, AppCommFactory) {
        $scope.appBlockwaiting = false;

        var onSetBlockwaiting = function(blockwait) {
            $scope.appBlockwaiting = blockwait;
        };

        function onDestroy() {
            AppCommFactory.events.off('setBlockwaiting', onSetBlockwaiting);
        }

        AppCommFactory.events.on('setBlockwaiting', onSetBlockwaiting);
        $scope.$on('$destroy', onDestroy);
    }

    function originShellBlockwait(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/blockwait.html'),
            controller: OriginShellBlockwaitCtrl,
            controllerAs: 'blockwaitCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellBlockwait
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Darkens the screen and shows the spinner until the application
     * is ready for the user. For example, this is shown when the user
     * loses authentication and the app performs automatic retry.
     * During the retry period, there is no point in the user interacting
     * with Origin since auth is needed for most requests.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-blockwait>
     *         </origin-shell-blockwait>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginShellBlockwaitCtrl', OriginShellBlockwaitCtrl)
        .directive('originShellBlockwait', originShellBlockwait);
}());
