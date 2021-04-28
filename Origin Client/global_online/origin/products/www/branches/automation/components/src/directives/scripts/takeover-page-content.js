/**
 * @file scripts/takeover-page-content.js
 */
(function(){
    'use strict';

    function originTakeoverPageContent(ComponentsConfigFactory){
        function originTakeoverPageContentLink(scope, element, attrs, takeoverPageController){
            scope.isShown = true;
            function setVisibility(visiblity) {
                scope.isShown= visiblity;
            }

            if (takeoverPageController && _.isFunction(takeoverPageController.registerPageContent)) {
                takeoverPageController.registerPageContent(setVisibility);
            }
        }
        return {
            restrict: 'E',
            scope: {
            },
            transclude: true,
            link: originTakeoverPageContentLink,
            require: '^?originTakeoverPage',
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/takeover-page-content.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originTakeoverPageContent
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * take over container
     *
     * @example
     * <origin-takeover-page>
     *     <origin-takeover-page-content>
     *     </origin-takeover-page-content>
     * </origin-takeover-page>
     */
    angular.module('origin-components')
        .directive('originPageContent', originTakeoverPageContent);
}());
