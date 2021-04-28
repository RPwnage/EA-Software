/**
 * @file slideToggle.js
 */
(function() {
    'use strict';

    function OriginSettingsSlideToggleCtrl() {
    }

    function originSettingsSlideToggle(ComponentsConfigFactory) {
        function originSettingsSlideToggleLink(scope, elem) {
            if (scope.addclass){
                $(elem).find('.origin-settings-div').addClass(scope.addclass);
            }
        }
        return {
            restrict: 'E',
            controller: 'OriginSettingsSlideToggleCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('settings/views/slideToggle.html'),
            link: originSettingsSlideToggleLink,
            scope: {
                'togglecondition': '@',
                'toggleaction': '&',
                'addclass': '@'
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:slideToggle
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} titlestr the title string
     * @param {string} descriptionstr the description str
     * @description
     *
     * Settings section
     *
     */
    angular.module('origin-components')
        .controller('OriginSettingsSlideToggleCtrl', OriginSettingsSlideToggleCtrl)
        .directive('originSettingsSlideToggle', originSettingsSlideToggle);
}());