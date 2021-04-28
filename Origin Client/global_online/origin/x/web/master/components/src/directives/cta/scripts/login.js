/**
 * @file cta/login.js
 */

(function() {
    'use strict';
    var CONTEXT_NAMESPACE = 'origin-cta-login';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent"
    };

    function OriginCtaLoginCtrl($scope, DialogFactory, ComponentsLogFactory, AuthFactory, UtilFactory) {

        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.btnText = UtilFactory.getLocalizedStr($scope.btnText, CONTEXT_NAMESPACE, 'logincta');

        $scope.onBtnClick = function() {
            AuthFactory.events.fire('auth:login');
        };
    }

    function originCtaLogin(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaLoginCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaLogin
     * @restrict E
     * @element ANY
     * @scope
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {String} description The login in string to show
     * @description
     *
     * Load internal url call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-login type="primary" login-cta="Login in sucka"></origin-cta-login>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaLoginCtrl', OriginCtaLoginCtrl)
        .directive('originCtaLogin', originCtaLogin);
}());
