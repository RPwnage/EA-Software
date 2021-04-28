/*jshint strict: false */
define([
    'core/user',
    'core/auth',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
    'core/locale'
], function(user, auth, dataManager, urls, errorhandler, localeModule) {

    /**
     * @module module:search
     * @memberof module:Origin
     */

    /**
     * retrieve results from store search service
     * @param  {string} searchString string
     * @param  {Object} options
     * @param  {string} localeOverride the locale passed in e.g. 'en-US'
     * @param  {string} threeLetterCountryCodeOverride the three letter country passed in e.g.'USA'
     *
    */
    function searchStore(searchString , options, localeOverride, threeLetterCountryCodeOverride) {
        //Options Object required by store team for getting carousels data
        var params = '';
        if(Object.keys(options).length){
            params = Object.keys(options).map(function(key) {
                return key + '=' + encodeURIComponent(options[key]);
            }).join('&');
        }

        var endPoint = urls.endPoints.searchStore + '&' + params,
            locale = localeOverride? localeOverride: localeModule.locale(),
            threeLetterCountryCode = threeLetterCountryCodeOverride? threeLetterCountryCodeOverride:localeModule.threeLetterCountryCode(),
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'locale',
                    'val': locale.toLowerCase()
                }, {
                    'label': 'threeLetterCountry',
                    'val': threeLetterCountryCode.toLowerCase()
                }, {
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
     * @param  {page} search result start row number.  Starts at 0. Maximum 20 rows per call
     * @param  {options} options includes param required by the service
     */
    function searchPeople(searchString, start) {
        var endPoint = urls.endPoints.searchPeople,
            config = {
                atype: 'GET',
                headers: [{
                    'label': 'Accept',
                    'val': 'application/json'
                }],
                parameters: [{
                    'label': 'userId',
                    'val': user.publicObjs.userPid()
                }, {
                    'label': 'searchkeyword',
                    'val': searchString
                },{
                    'label': 'start',
                    'val': start
                }],
                appendparams: [],
                reqauth: true,
                requser: true
            },
            token = user.publicObjs.accessToken();

        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.dataRESTauth(endPoint, config)
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
