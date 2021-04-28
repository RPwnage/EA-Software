(function () {
    "use strict";

    var SEARCH_PROVIDER_AREA = 'store';

    function StoreSearchProvider() {

        return {
            search: function (rec, id, options) {

                Origin.search.search(rec.searchString, options, function (data) {
                    var result = JSON.parse(data).result.games;
                    result.length = parseInt(result.numFound, 10);
                    rec.set(rec, SEARCH_PROVIDER_AREA, id, result);
                }, function () {
                    rec.set(rec, SEARCH_PROVIDER_AREA, id, []);
                });
            },


//                setTimeout(function () {
//                    var data = [],
//                        entitlements = GamesDataFactory.baseGameEntitlements(),
//                        entitlement,
//                        offers = GamesDataFactory.catalog(),
//                        offer,
//                        searchTokens = rec.searchString.toLowerCase().split(' '),
//                        searchToken,
//                        findTokens,
//                        findToken,
//                        found;
//
//                    for (entitlement in entitlements) {
//                        if (entitlements.hasOwnProperty(entitlement)) {
//                            offer = offers[entitlements[entitlement].offerId];
//                            if (offer) {
//                                findTokens = offer.displayName.toLowerCase().split(' ');
//
//                                for (searchToken in searchTokens) {
//                                    if (searchTokens.hasOwnProperty(searchToken)) {
//                                        found = false;
//                                        for (findToken in findTokens) {
//                                            if (findTokens.hasOwnProperty(findToken)) {
//                                                if (findTokens[findToken].indexOf(searchTokens[searchToken]) >= 0) {
//                                                    found = true;
//                                                    break;
//                                                }
//                                            }
//                                        }
//                                        if (!found) {
//                                            break;
//                                        }
//                                    }
//                                }
//
//                                if (found) {
//                                    data.push({ offerId: entitlements[entitlement].offerId});
//                                }
//                            }
//                        }
//                    }
//
//                    rec.set(rec, SEARCH_PROVIDER_AREA, id, data);
//
//                }, 3000);
//            },

            area: SEARCH_PROVIDER_AREA
        };
    }

    angular.module('origin-components')
        .factory('StoreSearchProvider', StoreSearchProvider);
}());