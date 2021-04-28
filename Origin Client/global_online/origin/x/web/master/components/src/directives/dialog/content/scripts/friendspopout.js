/**
 * @file dialog/content/Friendspopout.js
 */
(function() {
    'use strict';

    function OriginDialogContentFriendspopoutCtrl($scope) {
        $scope.friendsListStr = $scope.friendsListStr || '';
        $scope.friends = $scope.friendsListStr.split(',');
    }

    function originDialogContentFriendspopout(ComponentsConfigFactory) {


        return {
            restrict: 'E',
            scope: {
                friendsPopoutTitle: '@friendspopouttitle',
                friendsListStr: '@friendsliststr'
            },
            controller:'OriginDialogContentFriendspopoutCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/friendspopout.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentFriendspopout
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} friendspopoutgame messaging for friends playing a game
     * @param {string} offerid the offer id of the game
     * @description
     *
     * friends played dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-friendspopout
     *             Friendspopoutgame="Friends Playing %game%"
     *             offerid="OFB-EAST:52735">
     *         </origin-dialog-content-Friendspopout>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginDialogContentFriendspopoutCtrl', OriginDialogContentFriendspopoutCtrl)
        .directive('originDialogContentFriendspopout', originDialogContentFriendspopout);
}());