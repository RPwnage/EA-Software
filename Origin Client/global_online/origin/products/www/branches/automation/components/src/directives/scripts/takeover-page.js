/**
 * @file scripts/takeover-page.js
 */
(function(){
    'use strict';

    function originTakeoverPageCtrl($scope){
        var setPageContentVisibilityFn;

        /**
         * hide page content with takeover
         */
        this.hidePageContent = function(requireDigest){
            if (setPageContentVisibilityFn && _.isFunction(setPageContentVisibilityFn)){
                setPageContentVisibilityFn(false);
                if (requireDigest){
                    $scope.$digest();
                }
            }
        };

        /**
         * Show page content with takeover
         */
        this.showPageContent = function(requireDigest){
            setPageContentVisibilityFn(true);
            if (requireDigest){
                $scope.$digest();
            }
        };

        this.registerPageContent = function(setPageContentVisibility){
            setPageContentVisibilityFn = setPageContentVisibility;
        };
    }

    function originTakeoverPage(ComponentsConfigFactory){
        return {
            restrict: 'E',
            scope: {
            },
            transclude: true,
            controller: originTakeoverPageCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('views/takeover-page.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originTakeoverPage
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * take over container
     *
     * @example
     * <origin-takeover-page>
     * </origin-takeover-page>
     */
    angular.module('origin-components')
        .directive('originTakeoverPage', originTakeoverPage);
}());
