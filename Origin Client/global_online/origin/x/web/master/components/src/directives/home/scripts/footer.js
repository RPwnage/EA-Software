/** 
 * @file home/footer.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-home-footer';

    function OriginHomeFooterCtrl($scope, UtilFactory) {
        $scope.title = UtilFactory.getLocalizedStr($scope.titleStr, CONTEXT_NAMESPACE, 'title');
        $scope.description = UtilFactory.getLocalizedStr($scope.descriptionStr, CONTEXT_NAMESPACE, 'description');
    }

    function originHomeFooter(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                descriptionStr: '@description',
                titleStr: '@title'
            },
            controller: 'OriginHomeFooterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('home/views/footer.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originHomeFooter
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title the title string
     * @param {LocalizedText} description the description str
     * @description
     *
     * home section footer
     *
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-home-footer titlestr="Looking for more?" descriptionstr="The more you play games and the more you add friends, the better your recommendations will become."></origin-home-footer>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginHomeFooterCtrl', OriginHomeFooterCtrl)
        .directive('originHomeFooter', originHomeFooter);
}());
