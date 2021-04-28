
/**
 * @file store/carousel/scripts/featuredsearch.js
 */
(function(){
    'use strict';
    var SEARCH_PROVIDER= 'store';
    var sortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };
    var SETUP_EVENT = 'carousel:setup';

    /**
    * The directive
    */
    function originStoreCarouselFeaturedsearch(ComponentsConfigFactory, SearchFactory) {

        var searchDefaults = {
            'searchString': '',
            'facetData': '',
            'orderBy': 'title',
            'direction': sortDirectionEnumeration.ascending,
            'limit': 30,
            'offeset': 0
        };

        function setDefaults(scope) {
            scope.direction = sortDirectionEnumeration[scope.direction];
            for(var param in searchDefaults) {
                if(scope[param] === undefined) {
                    scope[param] = searchDefaults[param];
                }
            }
        }

        /**
        * The directive link
        */
        function originStoreCarouselFeaturedsearchLink(scope, elem) {
            var searchCtx;
            // Add Scope functions and call the controller from here
            setDefaults(scope);
            searchCtx = SearchFactory.createSearchContext();
            searchCtx.events.on('search:finished', function() {
                var results = searchCtx.getResults(SEARCH_PROVIDER);
                scope.offers = results.game;
                scope.$digest();
                //Trigger the event on the slider.
                elem.find('.origin-store-carousel-featured-carousel-slider').trigger(SETUP_EVENT);
            });

            var params = {
                fq: scope.facetData,
                sort: scope.orderBy + ' ' + scope.direction,
                start: scope.offset,
                rows: scope.limit
            };

            SearchFactory.search(searchCtx, {
                searchString: scope.searchString,
                includes: scope.includes,
                store: params
            });
        }
        return {
            restrict: 'E',
            replace: true,
            scope: {
                title: '@',
                text: '@',
                description: '@',
                href: '@',
                imageSrc: '@',
                searchString: '@',
                facetData: '@',
                orderBy: '@',
                direction: '@',
                limit: '@',
                offset: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/featuredsearch.html'),
            link: originStoreCarouselFeaturedsearchLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreCarouselFeaturedsearch
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * @param {LocalizedString} title The title of the module
     * @param {LocalizedString} text Descriptive text describing the products
     * @param {LocalizedString} description The text in the call to action
     * @param {Url} href the The destination of the cta
     * @param {ImageUrl} imageSrc The sorce for the image
     * @param {string} searchString The keyword to do the search for.
     * @param {string} facetData JSON array of data of the form {platform: [pc-dowlnload], genre: [shooter], gametype: [basegame]}
     * @param {sortDirectionEnumeration} direction The direction to sort the data
     * @param {string} orderBy the feild to do the ordering on. Should be one of the data fields returned by solr.
     * @param {LocalizedString} title Main title for the component
     * @param {LocalizedString} viewAllStr The text for the view all link
     * @param {number} limit The max number of elements that should be returned.
     * @param {number} offset  The offset into the result set.
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-featuredsearch />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .directive('originStoreCarouselFeaturedsearch', originStoreCarouselFeaturedsearch);
}());
