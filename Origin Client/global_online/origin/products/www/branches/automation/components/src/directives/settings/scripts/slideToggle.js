/**
 * @file slideToggle.js
 */
(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'slide-toggle';

    function OriginSettingsSlideToggleCtrl($scope, UtilFactory) {
        $scope.slideToggleOnLoc = UtilFactory.getLocalizedStr($scope.slideToggleOnStr, CONTEXT_NAMESPACE, 'slide-toggle-on-str');
        $scope.slideToggleOffLoc = UtilFactory.getLocalizedStr($scope.slideToggleOffStr, CONTEXT_NAMESPACE, 'slide-toggle-off-str');

        $scope.toggleactionlocal = function() {
            if($scope.togglecondition === 'true') {
                $scope.togglecondition = 'false';
            }
            else {
                $scope.togglecondition = 'true';
            }
            $scope.toggleaction();
        };
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
                'toggledisabled': '=',
                'addclass': '@',
                'slideToggleOnStr': '@',
                'slideToggleOffStr': '@'
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:slideToggle
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} slide-toggle-on-str *update in defaults
     * @param {LocalizedString} slide-toggle-off-str *update in defaults
     * @description
     *
     * Settings section
     *     
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-settings-slide-toggle 
     *              slide-toggle-on-str="On"
     *              slide-toggle-off-str="Off"
     *          </origin-settings-slide-toggle>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginSettingsSlideToggleCtrl', OriginSettingsSlideToggleCtrl)
        .directive('originSettingsSlideToggle', originSettingsSlideToggle);
}());