/**
 * @file pdp-url.js
 */

(function() {
    'use strict';

    /**
     * @ngdoc filter
     * @name origin-components.filters:pdpUrl
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Filter a product model and output the product URL
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *         <span>{{ model | pdpUrl }}</span>
     *     </file>
     * </example>
     *
     */
    function pdpUrlFilter($state) {
        return function (offer) {
            var stateName, urlParameters, url;

            if (!offer) {
                return '';
            }

            if (angular.isDefined(offer.franchiseKey)) {
                stateName = 'app.store.wrapper.pdp';
                urlParameters = {
                    franchise: offer.franchiseKey,
                    game: offer.gameKey,
                    edition: offer.editionKey
                };
            } else {
                stateName = 'app.store.wrapper.oldpdp';
                urlParameters = {
                    offerId: offer.offerId
                };
            }

            url = $state.href(stateName, urlParameters).substring(1);

            return url;
        };
    }

    angular.module('origin-components')
        .filter('pdpUrl', pdpUrlFilter);
}());
