(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-search-logged-out';

    /**
     * Global Search Logged Out Ctrl
     */
    function OriginSearchLoggedOutCtrl($scope, UtilFactory, AuthFactory) {

        /* Get Translated Strings */
        $scope.loggedOutTitleLoc = UtilFactory.getLocalizedStr($scope.loggedOutTitleStr, CONTEXT_NAMESPACE, 'loggedouttitle');
        $scope.logInButtonLabelLoc = UtilFactory.getLocalizedStr($scope.logInButtonLabelStr, CONTEXT_NAMESPACE, 'loginbuttonlabel');

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
                loggedOutTitleStr: '@loggedouttitle',
                logInButtonLabelStr: '@loginbuttonlabel'
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
     * @param {LocalizedString} loggedouttitle "Log in to search for games in your library"
     * @param {LocalizedString} loginbuttonlabel "Log In"
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