/**
 * @file store/pdp/scripts/feature-accoladehero-wrapper.js
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

    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-accolade-hero-wrapper';

    function originStorePdpFeatureAccoladeHeroWrapper(ComponentsConfigFactory, DirectiveScope) {

        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                quote1Str: '@',
                quote1SourceStr: '@',
                quote2Str: '@',
                quote2SourceStr: '@',
                quote3Str: '@',
                quote3SourceStr: '@',
                featureHeroImage: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            controller: 'originHeroWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/feature-accoladehero-wrapper.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureAccoladeHeroWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString} quote1-str Promo quote 1 - should be wrapped in quotes - use \&#8220; text \&#8221;
     * @param {LocalizedString} quote1-source-str Promo quote 1 source
     * @param {LocalizedString} quote2-str Promo quote 2 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote2-source-str Promo quote 2 source - not visible on small screens
     * @param {LocalizedString} quote3-str Promo quote 3 - not visible on small screens - should be wrapped in quotes - use  \&#8220; text \&#8221;
     * @param {LocalizedString} quote3-source-str Promo quote 3 source - not visible on small screens
     * @param {ImageUrl} feature-hero-image background image URL
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <origin-store-pdp-feature-accolade-hero-wrapper
     *             quote1-str="Awesome game"
     *             quote1-source-str="ign"
     *             feature-hero-image="https://qa.www.assets.cms.origin.com/content/dam/originx/web/app/home/actions/dai_intro_bg.png"
     *         </origin-store-pdp-feature-accolade-hero-wrapper>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureAccoladeHeroWrapper', originStorePdpFeatureAccoladeHeroWrapper);
}());