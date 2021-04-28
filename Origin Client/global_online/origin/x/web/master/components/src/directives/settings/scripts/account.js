/**
 * @file account.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-account';

    function OriginSettingsAccountCtrl($scope, UtilFactory, ComponentsConfigFactory, UserDataFactory, ComponentsLogFactory) {
        $scope.eaAcctTitleStr = UtilFactory.getLocalizedStr($scope.eaAcctTitleStr, CONTEXT_NAMESPACE, 'eaAcctTitleStr');
        $scope.editOnEATitleStr = UtilFactory.getLocalizedStr($scope.editOnEATitleStr, CONTEXT_NAMESPACE, 'editOnEATitleStr');
        $scope.originIdStr = UtilFactory.getLocalizedStr($scope.originIdStr, CONTEXT_NAMESPACE, 'originIdStr');
        $scope.passwordStr = UtilFactory.getLocalizedStr($scope.passwordStr, CONTEXT_NAMESPACE, 'passwordStr');
        $scope.secAndPrivTitleStr = UtilFactory.getLocalizedStr($scope.secAndPrivTitleStr, CONTEXT_NAMESPACE, 'secAndPrivTitleStr');
        $scope.secAndPrivTextStr = UtilFactory.getLocalizedStr($scope.secAndPrivTextStr, CONTEXT_NAMESPACE, 'secAndPrivTextStr');
        $scope.blocklistTitleStr = UtilFactory.getLocalizedStr($scope.blocklistTitleStr, CONTEXT_NAMESPACE, 'blocklistTitleStr');
        $scope.blocklistTextStr = UtilFactory.getLocalizedStr($scope.blocklistTextStr, CONTEXT_NAMESPACE, 'blocklistTextStr');
        $scope.unblockUserStr = UtilFactory.getLocalizedStr($scope.unblockUserStr, CONTEXT_NAMESPACE, 'unblockUserStr');
        $scope.loadingMoreStr = UtilFactory.getLocalizedStr($scope.loadingMoreStr, CONTEXT_NAMESPACE, 'loadingMoreStr');

		$scope.securityImgSrc = ComponentsConfigFactory.getImagePath('security-temp-img.png');

        $scope.loggedIn = Origin && Origin.auth && Origin.auth.isLoggedIn() || false;
        $scope.myNucleusId = $scope.loggedIn && Origin.user.userPid();
        $scope.myOriginId = $scope.loggedIn && Origin.user.originId();

		$scope.getOriginIdStr = function(){
			return $scope.originIdStr + ': ' + $scope.myOriginId;
		};
		$scope.getPasswordStr = function(){
			return $scope.passwordStr + ': ••••••••••••••••';
		};


		$scope.openURL = function(url){
			Origin.client.desktopServices.asyncOpenUrlWithEADPSSO(url);
		};

        function requestAvatar(nucleusId) {
            UserDataFactory.getAvatar(nucleusId, 'AVATAR_SZ_LARGE')
                .then(function(response) {
                    $scope.avatarImgSrc = response;
                    $scope.$digest();
                }, function() {

                }).catch(function(error) {
                    ComponentsLogFactory.error('originSettingsAccountCtrl: UserDataFactory.getAvatar failed', error);
                });
        }

        requestAvatar($scope.myNucleusId);
    }

    function originSettingsAccount(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@descriptionstr',
                titleStr: '@titlestr'
            },
            controller: 'OriginSettingsAccountCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/account.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsAccount
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsAccountCtrl', OriginSettingsAccountCtrl)
        .directive('originSettingsAccount', originSettingsAccount);
}());