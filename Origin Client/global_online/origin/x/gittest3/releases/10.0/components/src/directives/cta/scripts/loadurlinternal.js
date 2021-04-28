/**
 * @file cta/loadurlinternal.js
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

    function OriginCtaLoadurlinternalCtrl($scope, DialogFactory, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        $scope.onBtnClick = function() {
            DialogFactory.openAlert({
                id: 'web-go-to-internal-web',
                title: 'Going to an internal location',
                description: 'When the rest of OriginX is fully functional, you will be taken to another page within the application.',
                rejectText: 'OK'
            });

            //disable for now till we get more routes
            //$location.path($scope.href);

            //load internal url here with $scope.link
            ComponentsLogFactory.log('CTA: Loading Url Internally');
        };

    }

    function originCtaLoadurlinternal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                href: '@',
                btnText: '@description',
                type: '@'
            },
            controller: 'OriginCtaLoadurlinternalCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originCtaLoadurlinternal
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} description the button text
     * @param {ButtonTypeEnumeration} type can be one of two values 'primary'/'secondary'. this sets the style of the button
     * @param {Url} href the internal origin url you want to load
     * @description
     *
     * Load internal url call to action button
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-cta-loadurlinternal href="http://someoriginlink" description="Take me to internal page" type="primary"></origin-cta-loadurlinternal>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginCtaLoadurlinternalCtrl', OriginCtaLoadurlinternalCtrl)
        .directive('originCtaLoadurlinternal', originCtaLoadurlinternal);
}());