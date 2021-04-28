/**
 * @file dialog/content/aboutclient.js
 */
(function() {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-content-aboutclient';
    /**
     * About Client Dialog Controller
     * @controller originDialogContentAboutclientCtrl
     */
    function originDialogContentAboutclientCtrl($scope, UtilFactory, NavigationFactory) {
        $scope.releaseStr = UtilFactory.getLocalizedStr($scope.releasestr, CONTEXT_NAMESPACE, 'releasestr');
        //Function to open links passed in externally since href won't
        $scope.onLinkClick = function (linkUrl) {
            NavigationFactory.externalUrl(linkUrl, true);
        };
    }

    /**
     * About Client Dialog Directive
     * @directive originDialogContentAboutclient
     */
    function originDialogContentAboutclient(ComponentsConfigFactory, $compile) {
        return {
            restrict: 'E',
            scope: {
                eulastr: '@'
            },
            require: ['^originDialogContentPrompt', 'originDialogContentAboutclient'],
            controller: 'originDialogContentAboutclientCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/aboutclient.html'),
            link: function(scope) {
                //To open the links in an external browser while maintaining the format coming from the Ebisu_Strings file in the C++ client
                //We turn all the href= into the otka class with an ng-click that calls our onLinkClick function. Then we compile it because ng-bind-html won't.
                scope.eulaprocessed = scope.eulastr.replace(/href=/g, "class=\"otka\" href ng-click=onLinkClick(");
                scope.eulaprocessed = scope.eulaprocessed.replace(/">/g, "\")>");
                $('.origin-dialog-aboutclient-desc').html($compile(scope.eulaprocessed)(scope));
            }
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogContentAboutclient
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} releasestr the release notes link title in the about origin context menu
     * @param {string} eulastr this information comes from the EULA backend and is placed on the about origin context menu
     * @description
     *
     * About Client dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-dialog-content-aboutclient
     *             releasestr="Release Notes"
     *             eulastr="Eula text from the backend"
     *         ></origin-dialog-content-aboutclient>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogContentAboutclientCtrl', originDialogContentAboutclientCtrl)
        .directive('originDialogContentAboutclient', originDialogContentAboutclient);

}());