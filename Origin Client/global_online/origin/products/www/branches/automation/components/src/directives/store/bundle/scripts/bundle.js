/**
 * @file store/bundle/bundle.js
 */
(function(){
    'use strict';
    function originStoreBundle(ComponentsConfigFactory, $timeout, AppCommFactory, StoreSearchQueryFactory) {
        function originStoreBundleLink(scope){

            /**
             * Extract facet IDs from key value pair
             * @param keyPair
             * @returns {String} facet ID
             */
            function getFacetId(keyPair){
                if (angular.isDefined(keyPair) && angular.isArray(keyPair) && keyPair.length >= 2){
                    return keyPair[1];
                }else{
                    return '';
                }
            }

            var facetIds = _.map(StoreSearchQueryFactory.extractFacetValuesFromQuery(scope.filterQuery), getFacetId);
            AppCommFactory.events.fire('storeBundle:hideFacet', facetIds);
            // Get initial list of games upon load
            $timeout(scope.search, 0);
        }
        return {
            restrict: 'E',
            transclude: true,
            scope: {
                filterQuery: '@',
                customRank: '@'
            },
            link: originStoreBundleLink,
            controller: 'OriginStoreBrowseCtrl',
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/bundle/views/bundle.html')
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStoreBundle
     * @restrict E
     * @element ANY
     * @scope
     *
     * @param {string} filter-query solr facet filter query. e.g., genre:action,gameType:dlc OR genre(action AND mmo)
     * @param {CustomRank} custom-rank The custom solr rank used to order the sort.
     *
     * @description bundle sort and filter container
     *
     * @example
     * <origin-store-row>
     *     <origin-store-bundle></origin-store-bundle>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStoreBundle', originStoreBundle);
}());
