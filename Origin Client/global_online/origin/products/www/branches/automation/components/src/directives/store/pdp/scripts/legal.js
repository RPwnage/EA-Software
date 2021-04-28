/**
 * @file store/pdp/scripts/legal.js
 */
(function () {
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-store-pdp-legalnotes';

    function originStorePdpLegalnotes(ComponentsConfigFactory, UtilFactory, LocFactory) {

        function originStorePdpLegalnotesLink(scope) {
            scope.legalNotesText = UtilFactory.getLocalizedStr(scope.legalNotes, CONTEXT_NAMESPACE, 'legal-notes');
            scope.salesTax = UtilFactory.getLocalizedStr(scope.salesTax, CONTEXT_NAMESPACE, 'sales-tax');

            function compileLegalNotes() {
                var url = '<a href="' + scope.salesTaxUrl + '" class="otkc otkc-small external-link">' + scope.salesTax + '</a>';
                scope.legalNotesText = LocFactory.substitute(scope.legalNotesText, { '%salestaxurl%': url });
            }

            compileLegalNotes();

            var stopWatching = scope.$watch('legalNotes', function(newValue) {
                if (newValue) {
                    scope.legalNotesText = newValue;
                    compileLegalNotes();
                    stopWatching();
                }
            });
        }

        return {
            restrict: 'E',
            scope: {
                legalNotes : '@',
                salesTax: '@',
                salesTaxUrl: '@'
            },
            link: originStorePdpLegalnotesLink,
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/legal.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpLegalnotes
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedText} legal-notes merchanzized legal notes
     * @param {LocalizedText} sales-tax text for url to replace %salestaxurl% in legal notes
     * @param {Url} sales-tax-url url to the sales tax page
     *
     * @description
     *
     * @example
     * <origin-store-pdp-legalnotes content="HTML content">
     *
     * </origin-store-pdp-legalnotes>
     *
     */
    angular.module('origin-components')
        .directive('originStorePdpLegalnotes', originStorePdpLegalnotes);
}());
