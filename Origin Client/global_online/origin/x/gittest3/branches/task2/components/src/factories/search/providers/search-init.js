(function () {
    "use strict";
    
    function SearchProviderInit(
        SearchFactory,
        GamesSearchProvider,
        StoreSearchProvider,
        PeopleSearchProvider
    ) {
        
        return {
            init: function () {
                SearchFactory.addSearchProvider(GamesSearchProvider, {
                    name: GamesSearchProvider.area,
                    title: 'My Games',
                    url: '/#/gamedetails?offerId='
                });

                SearchFactory.addSearchProvider(StoreSearchProvider, {
                    name: StoreSearchProvider.area,
                    title: 'Store',
                    url: '/#/store?offerId='
                });
                
                SearchFactory.addSearchProvider(PeopleSearchProvider, {
                    name: PeopleSearchProvider.area,
                    title: 'People',
                    url: '/#/profile?offerId='
                });
            }
        };
    }
            
    angular.module('origin-components')
        .factory('SearchProviderInit', SearchProviderInit);
}());