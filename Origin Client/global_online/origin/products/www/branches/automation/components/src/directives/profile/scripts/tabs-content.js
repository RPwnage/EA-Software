(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileTabsContentCtrl($scope, $stateParams, WishlistFactory) {

        $scope.isWishlistEnabled = WishlistFactory.isWishlistEnabled();
        $scope.detailsId = $stateParams.detailsId;
        $scope.topic = $stateParams.topic;

    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileTabsContent
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} nucleusid - nucleusId of the user
     * @description
     *
     * Profile Tabs Content
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-tabs-content nucleusid="123456789"></origin-profile-tabs-content>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileTabsContentCtrl', OriginProfileTabsContentCtrl)
        .directive('originProfileTabsContent', function (ComponentsConfigFactory) {

            return {
                restrict: 'E',
                controller: 'OriginProfileTabsContentCtrl',
                scope: {
                    nucleusId: '@nucleusid'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/tabs-content.html'),
            };
        });
}());
