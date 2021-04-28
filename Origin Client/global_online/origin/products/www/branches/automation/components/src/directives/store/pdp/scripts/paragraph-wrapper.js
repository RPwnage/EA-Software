/**
 * @file store/pdp/scripts/description.js
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

    var CONTEXT_NAMESPACE = 'origin-store-pdp-paragraph-wrapper';

    function originStorePdpParagraphWrapper(ComponentsConfigFactory, DirectiveScope) {
        return {
            restrict: 'E',
            scope: {
                titleStr: '@',
                anchorName: '@',
                showOnNav: '@',
                headerTitle: '@',
                paragraphText: '@'
            },
            link: DirectiveScope.populateScopeLinkFn(CONTEXT_NAMESPACE),
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/paragraph-wrapper.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpParagraphWrapper
     * @restrict E
     * @element ANY
     * @scope
     * @description wrapper to hide/show header/nav item when no data is available
     * @param {LocalizedString} title-str the title of the navigation item
     * @param {string} anchor-name the hash anchor name to link to
     * @param {BooleanEnumeration} show-on-nav determines whether or not this navigation item should show on a navigation bar
     * @param {LocalizedString} header-title title of this section
     * @param {LocalizedString|OCD} paragraph-text html content for paragraph
     *
     * @example
     * <origin-store-pdp-paragraph-wrapper title-str="" anchor-name="" show-on-nav="true" header-title="" paragraph-text="">
     * </origin-store-pdp-paragraph-wrapper>
     *
     */
    angular.module('origin-components')
        .directive('originStorePdpParagraphWrapper', originStorePdpParagraphWrapper);

}());
