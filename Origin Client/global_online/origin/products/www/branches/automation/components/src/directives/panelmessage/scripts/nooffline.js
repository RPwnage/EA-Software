/**
 * @file panelmessage/nooffline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-panelmessage-nooffline';

    //controller
    function OriginPanelmessageNoofflineCtrl($scope, UtilFactory) {
        $scope.titleStr = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title-str');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionstr, CONTEXT_NAMESPACE, 'description');
    }

    //directive
    function originPanelmessageNooffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                descriptionstr: '@description'
            },
            controller: 'OriginPanelmessageNoofflineCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('panelmessage/views/panelmessage.html'),
        };
    }


    /**
     * @ngdoc directive
     * @name origin-components.directives:originPanelmessageNooffline
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-str the title text
     * @param {LocalizedText} description the description text
     * @description
     *
     * offline mode unavailable panel message
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-panelmessage-nooffline></origin-panelmessage-nooffline>
     *     </file>
     * </example>
     *
     */
    // directive declaration
    angular.module('origin-components')
        .controller('OriginPanelmessageNoofflineCtrl', OriginPanelmessageNoofflineCtrl)
        .directive('originPanelmessageNooffline', originPanelmessageNooffline);
}());