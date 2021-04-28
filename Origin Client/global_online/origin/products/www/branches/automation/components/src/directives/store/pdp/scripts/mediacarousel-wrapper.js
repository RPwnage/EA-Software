
/**
 * @file store/carousel/product/scripts/mediacarousel-wrapper.js
 */
(function(){
    'use strict';

    /**
     * BooleanTypeEnumeration list of allowed types
     * @enum {string}
     */
    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    function createWrapper(directiveTagName, templateUrl, transclude) {
        //must inject dependencies manually because ng-annotate cannot detect it.
        return ['ComponentsConfigFactory', 'DirectiveScope', function(ComponentsConfigFactory, DirectiveScope) {
            return {
                restrict: 'E',
                transclude: transclude,
                scope: {
                    titleStr: '@',
                    anchorName: '@',
                    showOnNav: '@',
                    headerTitle: '@'
                },
                link: DirectiveScope.populateScopeLinkFn(directiveTagName),
                templateUrl: ComponentsConfigFactory.getTemplatePath(templateUrl)
            };
        }];

    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpMediaCarouselWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     *
     *
     * @example
     *  <origin-store-pdp-media-carousel-wrapper>
     *  </origin-store-pdp-media-carousel-wrapper>
     */
    angular.module('origin-components')
        .directive('originStorePdpSystemrequirementsWrapper', createWrapper('origin-store-pdp-systemrequirements-wrapper', 'store/pdp/views/systemrequirements-wrapper.html', false))
        .directive('originStorePdpDetailsWrapper', createWrapper('origin-store-pdp-details-wrapper', 'store/pdp/views/details-wrapper.html', true))
        .directive('originStorePdpDescriptionWrapper', createWrapper('origin-store-pdp-description-wrapper', 'store/pdp/views/description-wrapper.html', false))
        .directive('originStorePdpOverviewWrapper', createWrapper('origin-store-pdp-overview-wrapper', 'store/pdp/views/overview-wrapper.html', false))
        .directive('originStorePdpMediaCarouselWrapper', createWrapper('origin-store-pdp-media-carousel-wrapper', 'store/pdp/views/mediacarousel-wrapper.html', true));
}());
