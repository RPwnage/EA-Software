/**
 * @file store/pdp/scripts/container-wrapper.js
 */
(function () {
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

    var CONTEXT_NAMESPACE = 'origin-store-pdp-feature-container-wrapper';

    function originStorePdpFeatureContainerWrapper(ComponentsConfigFactory, DirectiveScope) {
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            controller: 'originHeroWrapperCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/premier-wrapper.html')
        };
    }
    
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpFeatureContainerWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @example
     * <origin-store-pdp-feature-premier-wrapper title-str="" anchor-name="" show-on-nav="true" header-title="" >
     * </origin-store-pdp-feature-premier-wrapper>
     *
     */
    angular.module('origin-components')
        .directive('originStorePdpFeatureContainerWrapper', originStorePdpFeatureContainerWrapper);

}());
