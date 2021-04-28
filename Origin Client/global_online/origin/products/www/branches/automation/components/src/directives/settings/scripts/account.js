/**
 * @file account.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-settings-account';

    function OriginSettingsAccountCtrl($scope, UtilFactory, ComponentsConfigFactory, UserDataFactory, ComponentsLogFactory) {
        $scope.eaAcctTitleStr = UtilFactory.getLocalizedStr($scope.eaAcctTitleStr, CONTEXT_NAMESPACE, 'eaaccttitlestr');
        $scope.editOnEATitleStr = UtilFactory.getLocalizedStr($scope.editOnEATitleStr, CONTEXT_NAMESPACE, 'editoneatitlestr');
        $scope.originIdStr = UtilFactory.getLocalizedStr($scope.originIdStr, CONTEXT_NAMESPACE, 'originidstr');
        $scope.passwordStr = UtilFactory.getLocalizedStr($scope.passwordStr, CONTEXT_NAMESPACE, 'passwordstr');
        $scope.secAndPrivTitleStr = UtilFactory.getLocalizedStr($scope.secAndPrivTitleStr, CONTEXT_NAMESPACE, 'secandprivtitlestr');
        $scope.secAndPrivTextStr = UtilFactory.getLocalizedStr($scope.secAndPrivTextStr, CONTEXT_NAMESPACE, 'secandprivtextstr');
        $scope.blocklistTitleStr = UtilFactory.getLocalizedStr($scope.blocklistTitleStr, CONTEXT_NAMESPACE, 'blocklisttitlestr');
        $scope.blocklistTextStr = UtilFactory.getLocalizedStr($scope.blocklistTextStr, CONTEXT_NAMESPACE, 'blocklisttextstr');
        $scope.unblockUserStr = UtilFactory.getLocalizedStr($scope.unblockUserStr, CONTEXT_NAMESPACE, 'unblockuserstr');
        $scope.loadingMoreStr = UtilFactory.getLocalizedStr($scope.loadingMoreStr, CONTEXT_NAMESPACE, 'loadingmorestr');

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
            UserDataFactory.getAvatar(nucleusId, Origin.defines.avatarSizes.LARGE)
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
                eaAcctTitleStr: '@eaaccttitlestr',
                editOnEATitleStr: '@editoneatitlestr',
                originIdStr: '@originidstr',
                passwordStr: '@passwordstr',
                secAndPrivTitleStr: '@secandprivtitlestr',
                secAndPrivTextStr: '@secandprivtextstr',
                blocklistTitleStr: '@blocklisttitlestr',
                blocklistTextStr: '@blocklisttextstr',
                unblockUserStr: '@unblockuserstr',
                loadingMoreStr: '@loadingmorestr'
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
     * @param {LocalizedString} eaaccttitlestr Your EA Account
     * @param {LocalizedString} editoneatitlestr Edit on EA.com
     * @param {LocalizedString} originidstr Origin ID
     * @param {LocalizedString} passwordstr Password
     * @param {LocalizedString} secandprivtitlestr Security & Privacy
     * @param {LocalizedString} secandprivtextstr Manage how people can find you, set the information displayed on your profile under the ea.com security & privacy settings.
     * @param {LocalizedString} blocklisttitlestr Block List
     * @param {LocalizedString} blocklisttextstr Blocked Users will not be able to see your profile, online status, or send you messages.
     * @param {LocalizedString} unblockuserstr Unblock User
     * @param {LocalizedString} loadingmorestr Loading More Users
     * @description
     *
     * Settings section
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-account 
     *              eaaccttitlestr="Your EA Account"
     *              editoneatitlestr="Edit on EA.com"
     *              originidstr="Origin ID"
     *              passwordstr="Password"
     *              secandprivtitlestr="Security & Privacy"
     *              secandprivtextstr="Manage how people can find you, set the information displayed on your profile under the ea.com security & privacy settings."
     *              blocklisttitlestr="Block List"
     *              blocklisttextstr="Blocked Users will not be able to see your profile, online status, or send you messages."
     *              unblockuserstr="Unblock User"
     *              loadingmorestr="Loading More Users">
     *         </origin-settings-account>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsAccountCtrl', OriginSettingsAccountCtrl)
        .directive('originSettingsAccount', originSettingsAccount);
}());