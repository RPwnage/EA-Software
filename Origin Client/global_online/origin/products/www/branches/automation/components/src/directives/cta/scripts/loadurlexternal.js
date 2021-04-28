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

    function OriginCtaLoadurlexternalCtrl($scope, NavigationFactory, ComponentsLogFactory) {
        $scope.iconClass = CLS_ICON_LEARNMORE;
        $scope.type = $scope.type || ButtonTypeEnumeration.primary;

        if ($scope.href) {
            $scope.btnHref = NavigationFactory.getAbsoluteUrl($scope.href);
        }

        $scope.onBtnClick = function($event) {
            $event.preventDefault();
            $event.stopPropagation();

            // Emit an event that is picked up by OriginHomeDiscoveryTileProgrammableCtrl,
            // in order to record cta-click telemetry for the tile.
            $scope.$emit('cta:clicked');

            NavigationFactory.externalUrl($scope.href, true);
            //load External url here with $scope.link
            ComponentsLogFactory.log('CTA: Loading Url Externally');
        };

    }

    function originCtaLoadurlexternal(ComponentsConfigFactory) {
        function originCtaLoadurlexternalLink(scope, element) {
            //we need to wait for the next frame for the class to be available from the DOM
            setTimeout(function() {
                //per design add an icon to the end of all load url external buttons
                var button = element.find('.otkbtn-primary-text');
                if (button) {
                    button.addClass('otkbtn-externallink');
                }
            }, 0);
        }

        return {
            restrict: 'E',
            scope: {
                href: '@',
                btnText: '@description',
                type: '@',
                icon: '@'
            },
            controller: 'OriginCtaLoadurlexternalCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('cta/views/cta.html'),
            link: originCtaLoadurlexternalLink
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