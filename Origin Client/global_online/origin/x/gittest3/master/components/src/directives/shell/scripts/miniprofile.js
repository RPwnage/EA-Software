/**
 * @file shell/scripts/miniprofile.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-shell-mini-profile';

    function OriginShellMiniProfileCtrl($scope, $rootScope, UtilFactory, LocFactory, AppCommFactory, DialogFactory, UserDataFactory) {
        $scope.loggedIn = Origin && Origin.auth && Origin.auth.isLoggedIn() || false;
        $scope.myNucleusId = $scope.loggedIn && Origin.user.userPid();
        $scope.myOriginId = $scope.loggedIn && Origin.user.originId();

        LocFactory.on('ready', function() {
            $scope.myProfileStr = UtilFactory.getLocalizedStr($scope.myProfileStr, CONTEXT_NAMESPACE, 'myprofilecta');
            $scope.accountStr = UtilFactory.getLocalizedStr($scope.accountStr, CONTEXT_NAMESPACE, 'accountcta');
            $scope.orderStr = UtilFactory.getLocalizedStr($scope.orderStr, CONTEXT_NAMESPACE, 'ordercta');
            $scope.logoutStr = UtilFactory.getLocalizedStr($scope.logoutStr, CONTEXT_NAMESPACE, 'logoutcta');
            $scope.pointsStr = UtilFactory.getLocalizedStr($scope.pointsStr, CONTEXT_NAMESPACE, 'pointscta', {
                '%points%': 0
            });
            $scope.loginOrRegister = UtilFactory.getLocalizedStr($scope.loginOrRegisterStr, CONTEXT_NAMESPACE, 'loginregister', {
                '%login%': '<a href="javascript:void(0)" class="otka" ng-click="login()">' + UtilFactory.getLocalizedStr($scope.loginStr, CONTEXT_NAMESPACE, 'logincta') + '</a>',
                '%register%': '<a href="javascript:void(0)" class="otka" ng-click="register()">' + UtilFactory.getLocalizedStr($scope.registerStr, CONTEXT_NAMESPACE, 'registercta') + '</a>'
            });
        });

        function updateScopeProperties(obj) {
            $scope.loggedIn = obj.isLoggedIn;
            $scope.myNucleusId = $scope.loggedIn && Origin.user.userPid();
            $scope.myOriginId = $scope.loggedIn && Origin.user.originId();
        }

        function onLogInOutNav(obj) {
            updateScopeProperties(obj);
            $scope.$digest();
        }

        function onDestroy() {
            AppCommFactory.events.off('auth:ready', onLogInOutNav);
            AppCommFactory.events.off('auth:change', onLogInOutNav);
        }

        $scope.login = function() {
            AppCommFactory.events.fire('auth:login');
        };

        $scope.logout = function() {
            AppCommFactory.events.fire('auth:logout');
        };

        $scope.register = function() {
            DialogFactory.openAlert({
                id: 'web-register-flow-not-done',
                title: 'Woops',
                description: 'The register flow has not been implemented.',
                rejectText: 'OK'
            });
        };

        $scope.profile = function() {
            location.href = "#/profile";
        };

        //event may already have been triggered; if so it's saved off in UserDataFactory
        if (UserDataFactory.isAuthReady()) {
            updateScopeProperties({
                isLoggedIn: UserDataFactory.isLoggedIn()
            });
        } else {
            //PJ: this ... seeems... stupid
            AppCommFactory.events.on('auth:ready', onLogInOutNav);
        }
        AppCommFactory.events.on('auth:ready', onLogInOutNav);
        AppCommFactory.events.on('auth:change', onLogInOutNav);
        $scope.$on('$destroy', onDestroy);

    }

    function originShellMiniProfile(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                'myProfileStr': '@myprofilecta',
                'loginOrRegisterStr': '@loginregister',
                'logoutStr': '@logoutcta',
                'loginStr': '@logincta',
                'registerStr': '@registercta',
                'orderStr': '@ordercta',
                'accountStr': '@accountcta',
                'pointsStr': '@pointscta'
            },
            controller: 'OriginShellMiniProfileCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('shell/views/miniprofile.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originShellMiniProfile
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} myprofilecta my profile action mini profile menu
     * @param {LocalizedString} loginregister login or register template mini profile menu
     * @param {LocalizedString} logoutcta logout action mini profile menu
     * @param {LocalizedString} logincta text login action mini profile menu
     * @param {LocalizedString} registercta register action mini profile menu
     * @param {LocalizedString} ordercta review order action mini profile menu
     * @param {LocalizedString} accountcta view account action mini profile menu
     * @param {LocalizedString} pointscta view points action mini profile menu
     * @description
     *
     * mini profile
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-shell-mini-profile></origin-shell-mini-profile>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginShellMiniProfileCtrl', OriginShellMiniProfileCtrl)
        .directive('originShellMiniProfile', originShellMiniProfile);

}());