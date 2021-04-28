/**
 * Created by handerson on 2015-04-21.
 * @file cta.js
 */
(function () {

    'use strict';

    function OriginSocialHubCtaCtrl($scope) {
        $scope = $scope;

        var $element;

        this.setElement = function (el) {
            $element = el;
        };

        // handle click event		
        this.ctaClick = function () {
            $element.trigger('buttonClicked');
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originSocialHubCta
     * @restrict E
     * @element ANY
     * @scope
     * @param {string} title the title of cta dialog
     * @param {string} paragraph the body of the cta dialog
     * @param {string} button the button text
     * @description
     *
     * origin social hub -> cta
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-social-hub-cta
     *            title="title"
     *            paragraph="paragraph"
     *            button="button"
     *            type="TYPE"
     *         ></origin-social-hub-cta>
     *     </file>
     * </example>
     *
     */

    angular.module('origin-components')
        .controller('OriginSocialHubCtaCtrl', OriginSocialHubCtaCtrl)
        .directive('originSocialHubCta', function(ComponentsConfigFactory) {
        
            return {
                restrict: 'E',
                controller: 'OriginSocialHubCtaCtrl',
                scope: {
                    'title': '@titletxt',
                    'paragraph': '@partxt',
                    'button': '@btntxt'
                },
                templateUrl: ComponentsConfigFactory.getTemplatePath('social/hub/views/cta.html'),
                link: function (scope, element, attrs, ctrl) {

                    ctrl.setElement($(element));

                    // Watch for clicks on the CTA button
                    $(element).on('click', '.origin-social-hub-cta-button', function (event) {
                        event = event;
                        ctrl.ctaClick();
                    });

                }
            };
        });
        
}());

