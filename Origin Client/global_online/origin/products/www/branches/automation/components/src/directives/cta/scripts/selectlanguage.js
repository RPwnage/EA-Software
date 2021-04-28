/**
 * @file cta/selectlanguage.js
 */

(function() {
    'use strict';

    function OriginCtaSelectlanguageCtrl($scope, DialogFactory) {

        // settings for the CTA
        $scope.type = 'secondary';
        $scope.href = '';
        $scope.icon = 'language';

        /**
        * Open up the language selection dialog on click
        * @method onBtnClick
        */
        $scope.onBtnClick = function() {
            DialogFactory.open({
                id: 'origin-dialog-content-selectlanguage',
                xmlDirective: '<origin-dialog-content-selectlanguage class="origin-dialog-content"></origin-dialog-content-selectlanguage>'
            });
        };

    }

    function originCtaSelectlanguage(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                btnText: '@description',
            },
            controller: 'OriginCtaSelectlanguageCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/icon.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaSelectlanguage
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @description
     *
     * Open up the select language dialog
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-selectlanguage  description="Take me to internal page"></origin-cta-selectlanguage>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaSelectlanguageCtrl', OriginCtaSelectlanguageCtrl)
        .directive('originCtaSelectlanguage', originCtaSelectlanguage);

}());
