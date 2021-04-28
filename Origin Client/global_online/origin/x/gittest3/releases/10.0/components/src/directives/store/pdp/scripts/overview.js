
/** 
 * @file store/pdp/scripts/overview.js
 */ 
(function(){
    'use strict';
    /**
    * The directive
    */
    function originStorePdpOverview(ComponentsConfigFactory, GamesDataFactory, $stateParams) {
        /**
        * The directive link
        */
        function originStorePdpOverviewLink(scope) {
            // TODO: will need to fetch catalog info based on offer path, rather than offerid
            if($stateParams.offerid) {
                GamesDataFactory.getCatalogInfo([$stateParams.offerid])
                    .then(function(data){
                        if (data){
                            scope.$evalAsync(function() {
                                // TODO: replace with catalog data

                                // overview
                                scope.overview = [
                                    {title: "Genre", value: '<a class="otka" href="#">Shooter</a>, <a class="otka" href="#">Action</a>'},
                                    {title: "Release Date", value: "October 13, 2013"},
                                    {title: "Rating", value: "ESRB: Mature"},
                                    {title: "Publisher", value: '<a class="otka" href="#">Electronic Arts</a>'},
                                    {title: "Developer", value: '<a class="otka" href="#">DICE</a>'},
                                    {title: "Supported Languages", value: "Deutsch (DE), English (US), Español (ES), Français (FR), Italiano (IT)"},
                                    {title: "Game Links", value: '<a class="otka" href="#">Official Site</a><br><a class="otka" href="#">Forums</a>'}
                                ];
                            });
                        }
                    })
                    .catch(function(error) {
                        Origin.log.exception(error, 'origin-store-pdp-overview - getCatalogInfo');
                    });
            }
        }
        return {
            restrict: 'E',
            scope: {},
            templateUrl: ComponentsConfigFactory.getTemplatePath('store/pdp/views/overview.html'),
            link: originStorePdpOverviewLink
        };
    }
    /**
     * @ngdoc directive
     * @name origin-components.directives:originStorePdpOverview
     * @restrict E
     * @element ANY
     * @scope
     * @description
     * @description PDP overview blocks, retrieved from catalog
     *
     *
     * @example
     * <origin-store-row>
     *     <origin-store-pdp-overview></origin-store-pdp-overview>
     * </origin-store-row>
     */
    angular.module('origin-components')
        .directive('originStorePdpOverview', originStorePdpOverview);
}()); 
