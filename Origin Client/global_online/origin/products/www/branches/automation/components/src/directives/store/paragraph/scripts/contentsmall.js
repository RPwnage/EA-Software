
/** 
 * @file store/paragraph/scripts/contentsmall.js
 */ 
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-contentsmall';

    function OriginStoreParagraphContentsmallCtrl($scope, UtilFactory) {
        $scope.strings = {
            description: UtilFactory.getLocalizedStr($scope.description, CONTEXT_NAMESPACE, 'description')
        };
    }

    function originStoreParagraphContentsmall() {
        return {
            restrict: 'E',
            replace: true,
            scope: {
            	description: '@'
            },
            controller: OriginStoreParagraphContentsmallCtrl,
            template: '<p class="otkc otkc-small" ng-bind-html="::strings.description"></p>'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphContentsmall
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} description The text for this paragraph.
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-paragraph-contentsmall
     *     		description="Some text.">
     *     </origin-store-paragraph-contentsmall>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreParagraphContentsmallCtrl', OriginStoreParagraphContentsmallCtrl)
        .directive('originStoreParagraphContentsmall', originStoreParagraphContentsmall);
}());