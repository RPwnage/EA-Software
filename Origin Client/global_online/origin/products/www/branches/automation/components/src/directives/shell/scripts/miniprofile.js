/**
 * @file shell/scripts/miniprofile.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-shell-miniprofile';

    function OriginShellMiniprofileCtrl($scope, $rootScope, $timeout, UtilFactory, AuthFactory, AchievementFactory, ComponentsLogFactory) {

        var delayOriginPtsPromise = null;

        $scope.myNucleusId = Origin.user.userPid;
        $scope.myOriginId = Origin.user.originId;
        $scope.hide = Origin.user.underAge;

        $scope.onClick = function() {
            $scope.$emit('miniprofile:toggle');
        };

        /**
         * Update Origin Points total
         * @method setOriginPoints
         */
        function setOriginPoints(achievementData) {
            $scope.pointsLoc = UtilFactory.getLocalizedStr($scope.pointsStr, CONTEXT_NAMESPACE, 'pointscta', {
                '%points%': achievementData.grantedXp()
            });
            $scope.$digest();
        }

        function delayOriginPoints() {
            delayOriginPtsPromise = null;
            AchievementFactory.getAchievementPortfolio()
                .then(setOriginPoints)
                .catch(function(error) {
                    ComponentsLogFactory.error('[Origin-Shell-Miniprofile getOriginPoints Method] error', error);
            });
        }

        /**
         * Get Origin Points from Achievement Factory
         * @method getOriginPoints
         */
        function getOriginPoints() {
            // this isn't critical and blocks the xhr calls so wait
            // until other stuff is done before populating

            delayOriginPtsPromise = $timeout(delayOriginPoints, 3000, false);
        }

        /**
         * Unhook from auth factory events when directive is destroyed.
         * @method onDestroy
         */
        function onDestroy() {
            AuthFactory.events.off('myauth:ready', getOriginPoints);
            if (delayOriginPtsPromise) {
                $timeout.cancel(delayOriginPtsPromise);
                delayOriginPtsPromise = null;
            }

        }

        /* Bind to auth events */
        AuthFactory.events.on('myauth:ready', getOriginPoints);

        $scope.$on('$destroy', onDestroy);

        if (AuthFactory.isAppLoggedIn()) {
            getOriginPoints();
        }
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
     * @param {LocalizedString} pointscta "%points% Origin Points"
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
