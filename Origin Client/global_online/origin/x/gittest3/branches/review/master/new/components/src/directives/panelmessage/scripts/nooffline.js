/**
 * @file panelmessage/nooffline.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-panelmessage-nooffline';

    //controller
    function OriginPanelmessageNoofflineCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'title');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionstr, CONTEXT_NAMESPACE, 'description');
    }

    //directive
    function originPanelmessageNooffline(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titlestr: '@title',
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
     * @param {LocalizedString} title the title text
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