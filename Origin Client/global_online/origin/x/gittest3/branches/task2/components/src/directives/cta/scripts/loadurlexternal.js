/**
 * @file cta/loadurlexternal.js
 */

(function() {
    'use strict';

    var CLS_ICON_LEARNMORE = 'otkicon-learn-more';

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

    function OriginCtaLoadurlexternalCtrl($scope, $window, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function() {
            $window.open($scope.href, '_blank');
            //load External url here with $scope.link
            ComponentsLogFactory.log('CTA: Loading Url Externally');
        };

    }

    function originCtaLoadurlexternal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                href: '@',
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaLoadurlexternalCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaLoadurlexternal
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {Url} href the external url you want to load
     * @description
     * Load external url call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-loadurlexternal href="http://espn.go.com" description="Read More From ESPN" type="primary"></origin-cta-loadurlexternal>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaLoadurlexternalCtrl', OriginCtaLoadurlexternalCtrl)
        .directive('originCtaLoadurlexternal', originCtaLoadurlexternal);
}());