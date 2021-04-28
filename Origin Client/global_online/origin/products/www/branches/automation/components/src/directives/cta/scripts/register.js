/**
 * @file cta/register.js
 */

(function() {
    'use strict';
    // TODO this needs to be updated to the correct key
    var CONTEXT_NAMESPACE = 'origin-cta-register';

    /**
     * A list of button type options
     * @readonly
     * @enum {string}
     */
    var ButtonTypeEnumeration = {
        "primary": "primary",
        "secondary": "secondary",
        "transparent": "transparent",
        "light": "light",
        "dark": "dark"
    };

    function OriginCtaRegisterCtrl($scope, DialogFactory, ComponentsLogFactory, AuthFactory, UtilFactory) {

        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.btnText = UtilFactory.getLocalizedStr($scope.btnText, CONTEXT_NAMESPACE, 'description');

        $scope.onBtnClick = function() {
            AuthFactory.events.fire('auth:register');
        };
    }

    function originCtaRegister(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                type: '@',
                btnText: '@description'
            },
            controller: 'OriginCtaRegisterCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaRegister
     * @restrict E
     * @element ANY
     * @scope
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {LocalizedString} description The register string you want to show
     *
     * @description
     *
     * Load internal url call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-register type="primary" description="Create Account"></origin-cta-register>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaRegisterCtrl', OriginCtaRegisterCtrl)
        .directive('originCtaRegister', originCtaRegister);
}());
