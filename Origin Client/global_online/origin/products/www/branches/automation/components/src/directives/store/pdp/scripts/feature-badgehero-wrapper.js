/**
 * @file store/pdp/scripts/feature-badgehero-wrapper.js
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

    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-badge-hero-wrapper';

    function originStorePdpFeatureBadgeHeroWrapper(ComponentsConfigFactory, DirectiveScope) {

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                quoteStr: '@',
                badgeImage: '@',
                featureHeroImage: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            controller: 'originHeroWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/feature-badgehero-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureBadgeHeroWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString} quote-str Promo quote
     * @param {ImageUrl} badge-image badge image URL
     * @param {ImageUrl} feature-hero-image background image URL
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-badge-hero-wrapper
     *            badge-image=""
     *            feature-hero-image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *         </origin-store-pdp-feature-badge-hero-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureBadgeHeroWrapper', originStorePdpFeatureBadgeHeroWrapper);
}());