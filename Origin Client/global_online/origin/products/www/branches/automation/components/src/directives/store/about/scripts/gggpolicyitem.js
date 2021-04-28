/**
 * @file store/about/scripts/gggpolicyitem.js
 */
(function () {
    'use strict';

    function originStoreAboutGggpolicyItem(ComponentsConfigFactory) {
        function originStoreAboutGggpolicyItemLink(scope, $element, attrs, ctrl){
            ctrl.registerPolicyItem(scope.date);
            scope.$watch(function(){
                return ctrl.getSelectedDate();
            }, function(selectedDate){
                scope.selectedDate = selectedDate;
            });
        }

        return {
            restrict: 'E',
            scope: {
                date: '@',
                content: '@'
            },
            require: '^originStoreAboutGggpolicy',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/gggpolicyitem.html'),
            link: originStoreAboutGggpolicyItemLink

        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutGggpolicyItem
     * @restrict E
     * @element ANY
     * @scope
     * @param {DateTime} date - date the policy was drafted (UTC time)
     * @param {LocalizedText} content - policy content
     *
     * @description A GGG policy item directive. Requires parent directive origin-store-about-gggpolicy.
     *
     * @example
     * <origin-store-about-gggpolicy>
     *     <origin-store-about-gggpolicy-item date="2016-02-01"
     *                                        content="test">
     * </origin-store-about-gggpolicy>
     */

    angular.module('origin-components')
        .directive('originStoreAboutGggpolicyItem', originStoreAboutGggpolicyItem);
}());
