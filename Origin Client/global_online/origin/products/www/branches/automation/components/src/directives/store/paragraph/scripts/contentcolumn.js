
/**
 * @file store/paragraph/scripts/contentcolumn.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-paragraph-contentcolumn';

    function OriginStoreParagraphContentcolumnCtrl($scope, UtilFactory) {
        $scope.strings = {
            content: UtilFactory.getLocalizedStr($scope.content, CONTEXT_NAMESPACE, 'content')
        };
    }

    function originStoreParagraphContentcolumn(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
            	content: '@'
            },
            controller: OriginStoreParagraphContentcolumnCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/paragraph/views/contentcolumn.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreParagraphContentcolumn
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @param {LocalizedString} content The text for this paragraph
     *
     * @description Provide a paragraph styled in columns format
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-paragraph-contentcolumn
     *     		content="Some text.">
     *     </origin-store-paragraph-contentcolumn>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreParagraphContentcolumnCtrl', OriginStoreParagraphContentcolumnCtrl)
        .directive('originStoreParagraphContentcolumn', originStoreParagraphContentcolumn);
}());
