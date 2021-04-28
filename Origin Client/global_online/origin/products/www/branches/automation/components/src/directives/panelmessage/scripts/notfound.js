/**
 * @file panelmessage/notfound.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-panelmessage-notfound';

    function originPanelmessageNotfound(ComponentsConfigFactory, UtilFactory, OriginDomHelper) {

        function originPanelmessageNotfoundLink(scope) {

            // add 404 meta tag for use with pre-render
            OriginDomHelper.addTag('head', 'meta', {name: 'prerender-status-code', content: '404'});

            scope.errorcode = UtilFactory.getLocalizedStr(scope.errorcodestr, CONTEXT_NAMESPACE, 'errorcode');
            scope.titleStr = UtilFactory.getLocalizedStr(scope.titlestr, CONTEXT_NAMESPACE, 'title-str');
            scope.description = UtilFactory.getLocalizedStr(scope.descriptionstr, CONTEXT_NAMESPACE, 'description');
            scope.isUnderage = Origin.user.underAge();
        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                errorcodestr: '@errorcode',
                descriptionstr: '@description'
            },
            link: originPanelmessageNotfoundLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('panelmessage/views/notfound.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originPanelmessageNotfound
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} errorcode error code text
     * @param {LocalizedString} title-str the title text
     * @param {LocalizedText} description the description text
     *
     * @description 404 panel message
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
        .directive('originPanelmessageNotfound', originPanelmessageNotfound);
}());