/*jshint unused: false */
/*jshint strict: false */
define([
    'core/user',
    'core/auth',
    'core/dataManager',
    'core/config',
    'core/urls',
    'core/errorhandler',
], function(user, auth, dataManager, config, urls, errorhandler) {

    /**
     * @module module:search
     * @memberof module:Origin
     */

    /**
     * retrieve results from store search service
     * @param  {searchString} search string
     * @param  {options} options includes param required by the service
     */
    function searchStore(searchString , options) {

        //TODO: Move the endpoint to URLS file once we get the services for XSearch up and running

        //Options Object required by store team for getting carousels data
        var params = '';
        if(Object.keys(options).length){
            params = Object.keys(options).map(function(key) {
                return key + '=' + encodeURIComponent(options[key]);
            }).join('&');
        }

        var locale = 'en_US',
            storeId = 'ww',
            env = 'author',
            endPoint = urls.endPoints.searchStore+'&'+params,
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'locale',
                    'val': locale
                },{
                    'label': 'storeId',
                    'val': storeId
                },{
                    'label': 'env',
                    'val': env
                },{
                    'label': 'q',
                    'val': searchString
                }],
                appendparams: [],
                reqauth: false,
                requser: false
            };

            return dataManager.dataREST(endPoint, config)
              .catch(errorhandler.logAndCleanup('STORE SEARCH FAILED'));

    }

    /**
     * retrieve results from people search service
     * @param  {searchString} search string
     * @param  {page} search result page number.  Starts at 1.  There are currently 20 results per page.    
     * @param  {options} options includes param required by the service
     */
    function searchPeople(searchString, page) {

        //TODO: Move the endpoint to URLS file once we get the services for XSearch up and running
        var locale = 'en_US',
            storeId = 'ww',
            env = 'author',
            endPoint = urls.endPoints.searchPeople,
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'authToken',
                    'val': user.publicObjs.accessToken()
                },{
                    'label': 'searchkeyword',
                    'val': searchString
                },{
                    'label': 'currentPageNo',
                    'val': page
                }],
                appendparams: [],
                reqauth: false,
                requser: false
            };

        return dataManager.dataREST(endPoint, config)
          .catch(errorhandler.logAndCleanup('PEOPLE SEARCH FAILED'));

    }

    return /** @lends module:Origin.module:search */ {

        /**
         * This will return a promise for the requested searchString from store search service
         *
         * @param  {string} searchString    searchString
         * @param  {object} options         object of params {fq: 'gameType:basegame',sort: 'title asc',start: 0,rows: 20}
         * @return {promise} return a promise with results based on the search string
         * @method
         */
        searchStore: searchStore,

        /**
         * This will return a promise for the requested searchString from people search service
         *
         * @param  {string} searchString    searchString 
         * @param  {number} page            page from which we want the result - for initial call its 1
         * @return {promise} return a promise with results based on the search string
         * @method
        */
        searchPeople: searchPeople

    };
});
