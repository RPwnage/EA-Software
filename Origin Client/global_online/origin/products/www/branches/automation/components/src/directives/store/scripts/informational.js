
/**
 * @file store/scripts/informational.js
 */
(function(){
    'use strict';

    /* jshint ignore:start */
    var TextSizeEnumeration = {
        "small": "small",
        "medium": "medium",
        "large": "large"
    };

    var ContentWidthEnumeration = {
        "small": "small",
        "large": "large"
    };

    var TextAlignmentEnumeration = {
        "left": "left",
        "center": "center",
        "right": "right"
    };
    /* jshint ignore:end */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };

    var CONTEXT_NAMESPACE = 'origin-store-informational';

    function originStoreInformational(ComponentsConfigFactory, DirectiveScope, NavigationFactory) {

        function originStoreInformationalLink(scope){
            scope.openLink = function(href, event){
                event.preventDefault();
                NavigationFactory.openLink(href);
            };

            DirectiveScope.populateScope(scope, CONTEXT_NAMESPACE).then(function(){
                scope.hrefAbsoluteUrl = NavigationFactory.getAbsoluteUrl(scope.href);
                scope.useH1 = scope.useH1Tag === BooleanEnumeration.true;
                scope.useH2 = !scope.useH1;
            });
        }

        return {
            restrict: 'E',
            scope: {
                headerText: '@',
                headerTextSize: '@',
                content: '@',
                quote: '@',
                href: '@',
                ctaText: '@',
                contentWidth: '@',
                textAlignment: '@',
                useH1Tag: '@'
            },
            link: originStoreInformationalLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/views/informational.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreInformational
     * @restrict E
     * @element ANY
     * @scope
     * @description A basic component for displaying informational content.
     * Has multiple configurations for header sizes, content, quotation, and cta.
     *
     * @param {LocalizedString} header-text The text for the header of the component
     * @param {TextSizeEnumeration} header-text-size The size of the header text
     * @param {TextAlignmentEnumeration} text-alignment Content alignment. Only affects content and quote.
     *
     * @param {LocalizedString} content The encoded html content for the component
     * @param {LocalizedString} quote An optional quote to display after the content
     * @param {LocalizedString} cta-text The text for the call to action
     * @param {Url} href The url for the cta.
     * @param {ContentWidthEnumeration} content-width The width of the content
     *
     * @param {BooleanEnumeration} use-h1-tag use H1 Tag for the title to boost SEO relevance. Optional and defaults to H2
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-informational header-text="This is the main text of the component" header-size="medium" href="/store"/ cta-text='Click Me'>
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreInformational', originStoreInformational);
}());
