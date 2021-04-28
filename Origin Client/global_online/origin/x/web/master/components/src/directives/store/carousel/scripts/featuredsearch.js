
/**
 * @file store/carousel/scripts/featuredsearch.js
 */
(function(){
    'use strict';

    var sortDirectionEnumeration = {
        "ascending": "asc",
        "descending": "desc"
    };

    var SETUP_EVENT = 'carousel:setup';
    var FINISHED_EVENT = 'carousel:finished';

    function OriginStoreCarouselFeaturedsearchCtrl ($scope, $element, $attrs, ObjectHelperFactory, SearchFactory, ComponentsLogFactory) {

        // set search-parameter defaults
        $scope.searchString = $attrs.searchString || '';
        $scope.facetData = $attrs.facetData || '';
        $scope.orderBy = $attrs.orderBy || 'title';
        $scope.direction = $attrs.direction || sortDirectionEnumeration.ascending;
        $scope.limit = $attrs.limit || 30;
        $scope.offset = $attrs.offset || 0;

        var searchParams = {
            fq: $scope.facetData,
            sort: $scope.orderBy + ' ' + $scope.direction,
            start: $scope.offset,
            rows: $scope.limit
        };

        function setOfferData(offers) {
            var carousel = $element.find('.origin-store-carousel-featured-carousel-slider');
            if (offers) {
                $scope.offers = offers;
                carousel.trigger(SETUP_EVENT);
            } else {
                $scope.offers = [];
                carousel.trigger(FINISHED_EVENT);
            }
        }

        function errorHandler(error) {
            ComponentsLogFactory.error('[origin-store-carousel-featuredsearch SearchFactory.searchStore] failed', error.message);
        }

        SearchFactory.searchStore($scope.searchString, searchParams)
            .then(ObjectHelperFactory.getProperty(['result', 'games', 'game']))
            .then(setOfferData)
            .catch(errorHandler);
    }


    /**
    * The directive
    */
    function originStoreCarouselFeaturedsearch(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            replace: true,
            scope: {
                title: '@',
                text: '@',
                description: '@',
                href: '@',
                imageSrc: '@',
                startColor: '@',
                endColor: '@'
            },
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/carousel/views/featuredsearch.html'),
            controller: OriginStoreCarouselFeaturedsearchCtrl
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
     * @param {ImageUrl} image-src The sorce for the image
     * @param {string} search-string The keyword to do the search for.
     * @param {string} facet-data JSON array of data of the form {platform: [pc-dowlnload], genre: [shooter], gametype: [basegame]}
     * @param {sortDirectionEnumeration} direction The direction to sort the data
     * @param {string} order-by the feild to do the ordering on. Should be one of the data fields returned by solr.
     * @param {LocalizedString} title Main title for the component
     * @param {LocalizedString} view-all-str The text for the view all link
     * @param {number} limit The max number of elements that should be returned.
     * @param {number} offset  The offset into the result set.
     * @param {string} start-color The start of the fade color for the background
     * @param {string} end-color The end color/background color of the module
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-store-carousel-featuredsearch />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('OriginStoreCarouselFeaturedsearchCtrl', OriginStoreCarouselFeaturedsearchCtrl)
        .directive('originStoreCarouselFeaturedsearch', originStoreCarouselFeaturedsearch);
}());
