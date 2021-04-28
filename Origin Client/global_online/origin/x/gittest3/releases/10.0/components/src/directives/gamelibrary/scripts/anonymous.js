/**
 * @file gamelibrary/anonymous.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-gamelibrary-anonymous';

    function OriginGamelibraryAnonymousCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.titlestr, CONTEXT_NAMESPACE, 'title');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionstr, CONTEXT_NAMESPACE, 'description');
    }

    function originGamelibraryAnonymous(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                titlestr: '@title',
                descriptionstr: '@description'
            },
            controller: 'OriginGamelibraryAnonymousCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('panelmessage/views/panelmessage.html'),
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originGameLibraryAnonymous
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the anonymouse user message title
     * @param {LocalizedText} description the anonymous user message description
     * @description
     *
     * game library anonymous container
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-gamelibrary-anonymous
     *              title="Games Library Unavailable"
     *              description="Log in to view your games">
     *         </origin-gamelibrary-anonymous>
     *     </file>
     * </example>
     *
     */

    // directive declaration
    angular.module('origin-components')
        .controller('OriginGamelibraryAnonymousCtrl', OriginGamelibraryAnonymousCtrl)
        .directive('originGamelibraryAnonymous', originGamelibraryAnonymous);
}());