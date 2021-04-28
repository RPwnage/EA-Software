/**
 * @file settingItem.js
 */
(function() {
    'use strict';

    function OriginSettingsItemCtrl() {
    }

    function originSettingsItem(ComponentsConfigFactory) {
        function originSettingsItemLink(scope, elem) {
            if (scope.nohint === 'true'){
                $(elem).find('.origin-settings-icon-wrap').addClass('origin-settings-hide');
            }
            if (scope.addclass){
                $(elem).find('.origin-settings-div.origin-settings-div-main').addClass(scope.addclass);
            }
        }

        return {
            restrict: 'E',
            controller: 'OriginSettingsItemCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/settingItem.html'),
            replace: true,
            transclude: true,
            link: originSettingsItemLink,
            scope: {
                'itemstr': '@',
                'itemhintstr': '@',
                'addclass': '@',
                'nohint': '@'
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSettingsItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} itemstr is the setting name string
     * @param {string} itemhintstr is the hint for the setting (optional)
     * @param {string} addclass is for any additional html class 
     * @param {bool} nohint is for if you don't want a tooltip with the item name
     * @description
     *
     * Settings section
     *
     */
    angular.module('origin-components')
        .controller('OriginSettingsItemCtrl', OriginSettingsItemCtrl)
        .directive('originSettingsItem', originSettingsItem);
}());