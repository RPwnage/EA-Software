
/** 
 * @file settingsfooter/scripts/link.js
 */ 
(function(){
    'use strict';

    function originSettingsfooterLinkCtrl ($scope, NavigationFactory) {
        $scope.onLinkClick = function () {
            NavigationFactory.asyncOpenUrlWithEADPSSO($scope.linkUrl, true);
        };
    }

    function originSettingsfooterLink(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                linkHtml: '@',
                linkUrl: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/link.html'),
            controller: originSettingsfooterLinkCtrl
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsfooterLink
     * @restrict E
     * @element ANY
     * @scope
     * @description Provisions a simple link in the settings footer
     * @param {Html} link-html The html for the footer link
     * @param {Url} link-url The link for navigation
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-settingsfooter-link />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originSettingsfooterLink', originSettingsfooterLink);
}()); 
