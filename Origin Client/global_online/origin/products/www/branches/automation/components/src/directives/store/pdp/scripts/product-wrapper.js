/**
 * @file store/pdp/scripts/product-wrapper.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var CONTEXT_NAMESPACE = 'origin-store-pdp-carousel-product-wrapper';
    
    function originStorePdpCarouselProductWrapper(ComponentsConfigFactory, DirectiveScope) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                carouselTitle: '@',
                viewAllStr: '@',
                viewAllHref: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/product-wrapper.html'),
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE)
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpCarouselProductWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString} carousel-title Title of the product carousel
     * @param {LocalizedString} view-all-str text for the link that directs a user to another page (ex. solr search or browse page)
     * @param {string} view-all-href Url for the link that directs a user to another page (ex. solr search or browse page)
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-product-wrapper />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpCarouselProductWrapper', originStorePdpCarouselProductWrapper);
}());
