/**
 * @file store/carousel/scripts/productsearch.js
 */
(function () {
    'use strict';

    /* jshint ignore:start */
    var BooleanEnumeration = {
        "true": "true",
        "false": "false"
    };
    /* jshint ignore:end */

    var SortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };

    var OrderByEnumeration = {
        "dynamicPrice": "dynamicPrice",
        "offerType": "offerType",
        "onSale": "onSale",
        "rank": "rank",
        "releaseDate": "releaseDate",
        "title": "title"
    };

    var SETUP_EVENT = 'carousel:setup';
    var FINISHED_EVENT = 'carousel:finished';

    /**
     * The directive
     */
    function originStoreCarouselProductsearch(NavigationFactory, ComponentsConfigFactory, ComponentsLogFactory, ObjectHelperFactory, SearchFactory, AppCommFactory, PdpUrlFactory) {
        function originStoreCarouselProductsearchLink(scope, element, attrs, originStorePdpSectionWrapperCtrl) {


            // set search-parameter defaults
            scope.searchString = scope.searchString || '';
            scope.facetData = scope.facetData || '';
            scope.orderBy = OrderByEnumeration[scope.orderBy] || OrderByEnumeration.title;
            scope.direction = scope.direction || SortDirectionEnumeration.ascending;
            scope.limit = scope.limit || 30;
            scope.offset = scope.offset || 0;

            var searchParams = {
                filterQuery: scope.facetData,
                sort: scope.orderBy + ' ' + scope.direction,
                start: scope.offset,
                rows: scope.limit
            };

            function setOfferData(offers) {
                if (offers) {
                    scope.offers = offers;
                    // Tell the carousel to recalculate its sizing, as new items were added
                    element.trigger(SETUP_EVENT);

                    if (originStorePdpSectionWrapperCtrl && _.isFunction(originStorePdpSectionWrapperCtrl.setVisibility)) {
                        originStorePdpSectionWrapperCtrl.setVisibility(true);
                    }
                } else {
                    scope.offers = [];
                    // Tell the carousel "we're done whether there's items or not", in the case of an error or empty response,
                    // otherwise the carousel will show a "loading" spinner forever.
                    element.trigger(FINISHED_EVENT);
                }
            }

            function errorHandler(error) {
                ComponentsLogFactory.error('[origin-store-carousel-productsearch SearchFactory.searchStore] failed', error);
            }

            function fireCarouselResetupEvent(){
                AppCommFactory.events.fire('carousel:resetup');
            }

            // removes the current PDPs ocd path from the solr search response
            function rejectPdpPath(searchResponse) {
                if(NavigationFactory.isState('app.store.wrapper.pdp') || NavigationFactory.isState('app.store.wrapper.addon')) {
                    var ocdPath = PdpUrlFactory.buildPathFromUrl();
                    searchResponse = _.reject(searchResponse, {'path': ocdPath});
                }

                return searchResponse;
            }

            SearchFactory.searchStore(scope.searchString, searchParams)
                .then(ObjectHelperFactory.getProperty(['games', 'game']))
                .then(rejectPdpPath)
                .then(setOfferData)
                .then(fireCarouselResetupEvent)
                .catch(errorHandler);

        }

        return {
            restrict: 'E',
            replace: true,
            scope: {
                titleStr: '@',
                searchString: '@',
                facetData: '@',
                orderBy: '@',
                direction: '@',
                limit: '@',
                offset: '@',
                overrideVault: '@'
            },
            require: '?^originStorePdpSectionWrapper',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/productsearch.html'),
            link: originStoreCarouselProductsearchLink
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselProductsearch
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {string} search-string The keyword to do the search for.
     * @param {string} facet-data JSON data of the form-  platform: pc-download, genre: shooter, gameType: basegame
     * @param {SortDirectionEnumeration} direction The direction to sort the data
     * @param {OrderByEnumeration} order-by the field to do the ordering on. Should be one of the data fields returned by solr.
     * @param {LocalizedString} title-str Main title for the component
     * @param {number} limit The max number of elements that should be returned.
     * @param {number} offset  The offset into the result set.
     * @param {BooleanEnumeration} override-vault Override the "included with vault" text
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-productsearch />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselProductsearch', originStoreCarouselProductsearch);
}());
