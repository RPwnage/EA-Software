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

    function OriginCtaLoadurlinternalCtrl($scope, AppCommFactory, ComponentsLogFactory, NavigationFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;
        $scope.btnHref = NavigationFactory.getAbsoluteUrl($scope.sref || $scope.href);

        $scope.onBtnClick = function($event) {
            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked');
            $event.preventDefault();

            if($scope.sref) {
                $event.stopPropagation();
                AppCommFactory.events.fire('uiRouter:go', $scope.sref);

                ComponentsLogFactory.log('CTA: Loading Url Internally using state name');
            } else if ($scope.href) {
                NavigationFactory.internalUrl($scope.href);
                $event.stopPropagation();

                //load internal url here with $scope.link
                ComponentsLogFactory.log('CTA: Loading Url Internally');
            }
        };

    }

    function originCtaLoadurlinternal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                href: '@',
                btnText: '@description',
                type: '@',
                sref: '@'
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
