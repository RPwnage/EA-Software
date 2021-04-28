/**
 * @file panelmessage/notfound.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-panelmessage-notfound';

    function OriginPanelmessageNotfoundCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'title');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionstr, CONTEXT_NAMESPACE, 'description');
    }

    function originPanelmessageNotfound(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titlestr: '@title',
                descriptionstr: '@description'
            },
            controller: 'OriginPanelmessageNotfoundCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('panelmessage/views/panelmessage.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPanelmessageNotfound
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the title text
     * @param {LocalizedText} description the description text
     * @description
     *
     * 404 panel message
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-panelmessage-notfound></origin-panelmessage-notfound>
     *     </file>
     * </example>
     *
     */
    angular.module('origin-components')
        .controller('OriginPanelmessageNotfoundCtrl', OriginPanelmessageNotfoundCtrl)
        .directive('originPanelmessageNotfound', originPanelmessageNotfound);
}());