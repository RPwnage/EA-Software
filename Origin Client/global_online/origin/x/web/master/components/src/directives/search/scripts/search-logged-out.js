(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-global-search';

    /**
     * Global Search Logged Out Ctrl
     */
    function OriginSearchLoggedOutCtrl($scope, UtilFactory, AuthFactory) {

        /* Get Translated Strings */
        $scope.strings = {
            loggedOutTitle: UtilFactory.getLocalizedStr($scope.loggedOutTitleStr, CONTEXT_NAMESPACE, 'loggedouttitle'),
            logInButtonLabel:  UtilFactory.getLocalizedStr($scope.logInButtonLabelStr, CONTEXT_NAMESPACE, 'loginbuttonlabel')
        };

        $scope.logIn = function() {
            AuthFactory.events.fire('auth:login');
        };
    }

    /**
     * Global Search Logged Out Directive
     */
    function originSearchLoggedOut(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                loggedOutTitleStr: '@',
                logInButtonLabelStr: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('search/views/search-logged-out.html'),
            controller: 'OriginSearchLoggedOutCtrl'
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSearchLoggedOut
     * @restrict E
     * @scope
     * @param {LocalizedString} loggedOutTitleStr logged out message
     * @param {LocalizedString} logInButtonLabelStr login button label text
     * @description
     *
     * Global Search Logged Out Page
     * When a user is logged out from the SPA and uses global search, this directive appears before
     * search results (store results only when logged out), informing the user that they must login
     * to see more results, and providing a "Log In" button.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-search-logged-out></origin-search-logged-out>
     *     </file>
     * </example>
     */
    //declaration
    angular.module('origin-components')
        .controller('OriginSearchLoggedOutCtrl', OriginSearchLoggedOutCtrl)
        .directive('originSearchLoggedOut', originSearchLoggedOut);
}());