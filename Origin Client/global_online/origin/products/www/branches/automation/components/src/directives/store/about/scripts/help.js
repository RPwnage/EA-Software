/**
 * @file store/about/scripts/help.js
 */
(function () {
    'use strict';
    var CHILD_WRAPPER_CLASS_NAME = 'l-origin-store-column-half';

    /* Directive declaration */
    function originStoreAboutHelp(ComponentsConfigFactory) {
        /* Directive link function*/
        function originStoreAboutHelpLink(scope, element) {
            var $row = element.find('.l-origin-store-row'),
                $children = $row.children();

            function wrapChild($child) {
                var $column = angular.element('<div></div>').addClass(CHILD_WRAPPER_CLASS_NAME);
                $column.append($child);
                $row.append($column);
            }

            _.forEach($children, wrapChild);
        }

        return {
            restrict: 'E',
            scope: {},
            transclude: true,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/about/views/help.html'),
            link: originStoreAboutHelpLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreAboutHelp
     * @restrict E
     * @element ANY
     * @scope
     *
     * @description a wrapper for store about help module contents
     *
     *
     * @example
     * <origin-store-about-help>
     *     <any></any>
     * </origin-store-about-help>
     */
    angular.module('origin-components')
        .directive('originStoreAboutHelp', originStoreAboutHelp);
}());
