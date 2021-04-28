(function () {

    'use strict';

    /**
    * The controller
    */
    function OriginProfileBlockedCtrl($scope, UtilFactory) {

        var CONTEXT_NAMESPACE = 'origin-profile-blocked';

        $scope.blockedtitleLoc = UtilFactory.getLocalizedStr($scope.blockedtitleStr, CONTEXT_NAMESPACE, 'blockedtitlestr');
        $scope.blockedinfoLoc = UtilFactory.getLocalizedStr($scope.blockedinfoStr, CONTEXT_NAMESPACE, 'blockedinfostr');
        $scope.blockedlinkLoc = UtilFactory.getLocalizedStr($scope.blockedlinkStr, CONTEXT_NAMESPACE, 'blockedlinkstr');
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originProfileBlocked
     * @restrict E
     * @element ANY
     * @scope
     * @param {number} nucleusid nucleusid of the user
     * @param {LocalizedString} blockedtitlestr "This user is on your block list."
     * @param {LocalizedString} blockedinfostr "If you would like to see more information about them, remove them from your"
     * @param {LocalizedString} blockedlinkstr "block list"
     * @description
     *
     * Blocked Profile
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-profile-blocked nucleusid="123456789"
     *            blockedtitlestr="This user is on your block list"
     *            blockedinfostr="If you would like to see more information about them, remove them from your"
     *            blockedlinkstr="block list"
     *         ></origin-profile-blocked>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginProfileBlockedCtrl', OriginProfileBlockedCtrl)
        .directive('originProfileBlocked', function (ComponentsConfigFactory, NavigationFactory, ComponentsConfigHelper) {

            return {
                restrict: 'E',
                controller: 'OriginProfileBlockedCtrl',
                scope: {
                    nucleusId: '@nucleusid',
                    blockedtitleStr: '@blockedtitlestr',
                    blockedinfoStr: '@blockedinfostr',
                    blockedlinkStr: '@blockedlinkstr'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('profile/views/blocked.html'),
				link: function(scope, element) {
					scope.$element = $(element).find('.origin-profile-blocked');

					// clicked on the show block list link
					scope.$element.on('click', '.blocked-link', function () {
                        var url = ComponentsConfigHelper.getUrl('accountPrivacySettings');
                        url = url.replace("{locale}", Origin.locale.locale());
					    NavigationFactory.externalUrl(url, true);
					});
				}
			};

        });
}());

