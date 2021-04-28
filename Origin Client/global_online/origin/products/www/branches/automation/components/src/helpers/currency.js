/**
 * @file common/numbers.js
 */
(function() {
    'use strict';

    function CurrencyHelper(ComponentsLogFactory, CurrencyFormatter, LocFactory) {

        function getFormat(formatData) {
            var dialect = Origin.locale.locale().toLowerCase().replace('_', '-'),
                storefront = Origin.locale.threeLetterCountryCode().toLowerCase(),
                formatting = angular.extend(
                    formatData.country[storefront],
                    formatData.locale[dialect],
                    formatData.override[dialect + '-' + storefront]
                );
            formatting.decimalPlaces = formatting.decimalPlaces ? formatting.decimalPlaces : 0;
            formatting.decimalSeparator = formatting.decimalSeparator ? formatting.decimalSeparator : '';
            formatting.thousandSeparator = formatting.thousandSeparator ? formatting.thousandSeparator : '';
            formatting.format = formatting.format ? formatting.format : '--';
            return formatting;
        }

        function formatPrice(value, formatData) {
            var formatting = getFormat(formatData);
            var number = CurrencyFormatter.numberFormat(
                value,
                formatting.decimalPlaces,
                formatting.decimalSeparator,
                formatting.thousandSeparator
            );

            return LocFactory.substitute(formatting.format, {'%symbol%':formatting.symbol, '%value%': number});
        }

        function formatCurrency(value) {
            return CurrencyFormatter.getData()
                .then(_.partial(formatPrice, value))
                .catch(function() {
                    ComponentsLogFactory.error("[CurrencyHelper] Couldn't retrieve currency formats.");
                    return {};
                });
        }

        return {
            formatCurrency: formatCurrency
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.helpers.CurrencyHelpers

     * @description
     *
     * Numbers helper
     */
    angular.module('origin-components')
        .factory('CurrencyHelper', CurrencyHelper);

})();