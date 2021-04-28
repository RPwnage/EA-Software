/**
 * @file dialog/content/linkreact.js
 */
(function() {
    'use strict';

    /**
     * An attr that sends reactions to Origin Client with a href/keyword
     * @directive originDialogContentLinkreact
     */
    function originDialogContentLinkreact(AppCommFactory) {
        function originDialogContentLinkreactLink(scope, elem) {
            elem.bind('click', function(event) {
                var infoToClient = '';
                if(event.target.nodeName === 'A') {
                    infoToClient = event.target.href.replace('https://local.app.origin.com/', '');
                    AppCommFactory.events.fire('cppDialog:linkReact', {
                        href: infoToClient
                    });
                }
                return false;
            });
        }
        return {
            restrict: 'A',
            link: originDialogContentLinkreactLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentLinkreact
     * @restrict A
     * @element ANY
     * @scope
     * @description
     *
     * An attr that sends reactions to Origin Client with a href/keyword
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <a href='openOER' origin-dialog-content-linkreact></a>
     *     </file>
     * </example>
     */

    // register to angular
    angular.module('origin-components')
        .directive('originDialogContentLinkreact', originDialogContentLinkreact);

}());