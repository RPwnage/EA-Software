/* @file utagdata.js
 *
 * UTagDataModelFactory
 *
 */
(function () {
    'use strict';
    var LOGGED_IN_STATUS = 'Logged In';
    var LOGGED_OUT_STATUS = 'Logged Out';
    var PDP_ROUTES = ['app.store.wrapper.addon', 'app.store.wrapper.pdp', 'app.store.wrapper.skupdp'];

    function UTagDataModelFactory(LocaleFactory, AuthFactory, GamesDataFactory, ObjectHelperFactory, AppCommFactory, PdpUrlFactory) {
        var uTagData;
        var COUNTRY_CODE = Origin.locale.countryCode();
        var CURRENCY = LocaleFactory.getCurrency(COUNTRY_CODE);
        /* jshint camelcase:false */
        var DEFAULT_UTAG_DATA = {
            region: '',
            locale: Origin.locale.locale(),
            country: Origin.locale.countryCode().toLowerCase(),
            language: Origin.locale.locale().split('_', 1)[0] || '',
            userid: '',
            user_status: '',
            referring_site: '',
            mvt_experiment: '',
            mvt_variation: '',
            intcmp: '', //,
            transaction_id: '',
            order_date: '',
            payment_type: '',
            product_id: [], //Offer id
            product_sku: [], //Offer id
            product_name: [], //localized product name
            product_price: [], //price
            total_price: '', //populate by lockbox
            total_tax: '', //populate by lockbox
            total_shipping: '', //populate by lockbox
            currency: CURRENCY,
            product_quantity: '', //Legacy, put 1 for now
            promo_code: '', //populate by lockbox
            product_discount: '', // populate by lockbox
            promotion_applied: '', //populate by lockbox
            product_category: '', //Game type
            product_facets: '',
            ecommerce_platform: 'Origin', //Always Origin?
            platform: '', // platformFacetKey
            studio: '', //developerFacetKey
            game_name: '', //Localized product name
            franchise_unit: '', //Catalog
            mdm_franchise: '', //Catalog
            mdm_master_title: '', //Catalog
            mdm_master_title_id: '', //Catalog
            unlocalized_title: '', //Appears to be unused
            unlocalized_franchise: '' //Appears to be unused
        };
        var PRODUCT_LIST_ITEMS = ['product_id', 'product_sku', 'product_name', 'product_price']; //Product level items must always be in array

        /* jshint latedef:nofunc */
        //Maps utag data properties to catalog info getters
        var U_TAG_PROPERTY_TO_CATALOG_PROPERTY = {
            'product_id': 'offerId',
            'product_sku': 'offerId',
            'product_name': ['i18n', 'displayName'],
            'game_name': ['i18n', 'displayName'],
            'product_price': 'originalUnitPrice',
            'product_category': 'offerType',
            'franchise_unit': 'franchiseFacetKey',
            'mdm_franchise': 'franchiseFacetKey',
            'mdm_master_title': 'masterTitle',
            'mdm_master_title_id': 'masterTitleId',
            'platform': 'platformFacetKey',
            'studio': 'developerFacetKey',
            'product_facets': buildProductFacets
        };

        /*
         Keeping track of index for data removing and manipulation
         Currently each catalog is insert into a common utag object on independant fields. If more than 1 catalog is present, it's converted to an array
         We need the index to be tracked inorder to remove arbitrary offer ID

         CurrentIndex tracks the position of offer data in the utag data layer
         However, at index 0 it's not an array because we convert it back to string
         It's already used to track how many items we currently have
         */
        var indexByOfferId = {};
        var currentIndex = -1;

        /**
         * Initialize utag data layer
         */
        function initUtagData() {
            if (!window.hasOwnProperty('utag_data')) {
                window.utag_data = uTagData = {};
            } else {
                uTagData = window.utag_data;
            }

            _.extend(uTagData, DEFAULT_UTAG_DATA);

            watchRouteChange();
            registerEventsListeners();
        }

        /**
         * Register events listeners
         */
        function registerEventsListeners(){
            AuthFactory.events.on('myauth:ready', updateUserInfo);
            AuthFactory.events.on('myauth:change', updateAuthChange);
        }

        /**
         *  Update utag data base on utag login status
         */
        function updateUserInfo(status) {
            if (Origin && Origin.user) {
                uTagData.userid = String(Origin.user.userPid() || '');
                uTagData.user_status = status && status.isLoggedIn ? LOGGED_IN_STATUS : LOGGED_OUT_STATUS;
            }
        }

        /**
         * Update utag data when auth changes
         * @param status
         */
        function updateAuthChange(status) {
            if (!status.isLoggedIn) {
                uTagData.user_status = LOGGED_OUT_STATUS;
                updateUserInfo();
            } else {
                uTagData.user_status = LOGGED_IN_STATUS;
                uTagData.userid = '';
            }
        }

        /**
         * Get offer catalog
         * @param offerId
         * @returns {*}
         */
        function getCatalogInfo(offerId) {
            return GamesDataFactory.getCatalogInfo([offerId]);
        }

        /**
         * Get offer price
         * @param offerId
         * @returns {*}
         */
        function getPriceInfo(offerId){
            return GamesDataFactory.getPrice(offerId);
        }

        /**
         * Builds product facet key list
         * @param game
         */
        function buildProductFacets(game){
            return _.compact([game.platformFacetKey, game.franchiseFacetKey, game.gameEditionTypeFacetKey, game.numberOfPlayersFacetKey, game.genreFacetKey, game.publisherFacetKey, game.developerFacetKey]);
        }

        /**
         * Get catalog and add to u tag data layer field
         *
         * @param {Object} catalogData
         * @param {String | Function} catalogInfoGetter object path or function that returns the data
         * @param uTagKey
         */
        function addCatalogInfoIntoUtagData(catalogData, catalogInfoGetter, uTagKey){
            var uTagValue;
            if (_.isFunction(catalogInfoGetter)){
                uTagValue = catalogInfoGetter(catalogData);
            }else{
                uTagValue = String(ObjectHelperFactory.getProperty(catalogInfoGetter)(catalogData));
            }
            if (PRODUCT_LIST_ITEMS.indexOf(uTagKey) > -1){
                appendToArray(uTagData, uTagKey, uTagValue);
            }else{
                convertToArrayAndAppendNewValue(uTagData, uTagKey, uTagValue);
            }
        }

        /**
         * Converts existing value into array if more than 1 value
         * If already an array, append value
         *
         * @param object
         * @param property
         * @param newValue
         */
        function convertToArrayAndAppendNewValue(object, property, newValue) {
            newValue = newValue || '';

            if (currentIndex === -1){ //-1 denotes empty utag data layer
                object[property] = newValue;
            }else if (currentIndex === 0){ //0 denotes 1 item current exists but not an array
                object[property] = [object[property]];
                object[property].push(newValue);
            }else{
                if (angular.isArray(object[property])){
                    object[property].push(newValue);
                }
            }
        }

        /**
         * Converts existing value into array if more than 1 value
         * If already an array, append value
         *
         * @param object
         * @param property
         * @param newValue
         */
        function appendToArray(object, property, newValue){
            object[property] = ObjectHelperFactory.wrapInArray(object[property]);
            object[property].push(newValue);
        }

        /**
         * Append more catalog to utag data
         * @param game
         */
        function appendTagData(game) {
            _.forEach(U_TAG_PROPERTY_TO_CATALOG_PROPERTY, function (catalogInfoGetter, uTagKey) {
                addCatalogInfoIntoUtagData(game, catalogInfoGetter, uTagKey);
            });
        }

        /**
         * Remove catalog from utag data
         * @param uTagKey
         * @param index
         * @private
         */
        function spliceArrayByIndex(uTagKey, index) {
            var contentList = uTagData[uTagKey];
            if (angular.isArray(contentList) && index < contentList.length) {
                contentList.splice(index, 1);
                currentIndex--;
                //Convert it back to element itself if only 1 element
                if (contentList.length === 1) {
                    uTagData[uTagKey] = contentList.pop() || '';
                }
            }
        }

        /**
         * Remove all catalog entry from utag data by index
         * Index is tracked internally by offer id
         * @param index
         */
        function removeCatalogDataByIndex(index) {
            _.forEach(U_TAG_PROPERTY_TO_CATALOG_PROPERTY, function (catalogKey, uTagKey) {
                spliceArrayByIndex(uTagKey, index);
            });
        }

        /**
         * unset all utag catalog data
         */
        function unsetCatalogInfo() {
            _.forEach(U_TAG_PROPERTY_TO_CATALOG_PROPERTY, function (catalogKey, uTagKey) {
                if (_.isString(DEFAULT_UTAG_DATA[uTagKey])){
                    uTagData[uTagKey] = DEFAULT_UTAG_DATA[uTagKey];
                }else{ //Non primitive default needs to make a new copy
                    uTagData[uTagKey] = _.clone(DEFAULT_UTAG_DATA[uTagKey]);
                }

            });
            currentIndex = -1;
            indexByOfferId = {};
        }

        /**
         * Get offer catalog data and add it to utag data layer
         * @param offerId
         */
        function processOfferId(offerId) {
            //If offer catalog data was already added before, don't add again
            if (indexByOfferId[offerId] && indexByOfferId[offerId] >= 0) {
                return;
            }
            var catalogPromise = getCatalogInfo(offerId);
            var pricePromise = getPriceInfo(offerId);
            return Promise.all([catalogPromise, pricePromise]).then(_.spread(function(catalogData, priceData){
                var catalog = catalogData[offerId],
                    rating,
                    price;
                if (priceData && angular.isArray(priceData) && priceData.length > 0 && priceData[0]) {
                    rating = priceData[0].rating;
                }
                if (angular.isArray(rating) && rating.length > 0 && rating[0]) {
                    price = rating[0] || {};
                }
                catalogData = _.extend(catalog, price);
                appendTagData(catalogData);
                indexByOfferId[offerId] = ++currentIndex;
            }));
        }

        /**
         * Add new catalog data to the utag data layer
         * @param offerId
         */
        function addNewCatalogData(offerIds) {
            offerIds = ObjectHelperFactory.wrapInArray(offerIds);
            return _.map(offerIds, processOfferId);
        }

        /**
         * Remove utag catalog data by offer id
         * @param offerId
         */
        function removeCatalogData(offerId) {
            var index = indexByOfferId[offerId];
            if (index && index > 0) {
                removeCatalogDataByIndex(index);
            } else {
                unsetCatalogInfo();
            }
        }

        function fireUTagEvent(){
            var event = document.createEvent('Events');
            event.initEvent('uTagDataLayerReady', true, true);
            document.dispatchEvent(event);
        }

        /**
         * Watch route change for PDP pages
         */
        function watchRouteChange() {
            AppCommFactory.events.on('uiRouter:stateChangeSuccess', function (event, toState) {
                unsetCatalogInfo();
                if (PDP_ROUTES.indexOf(toState.name) > -1) {
                    var ocdPath = PdpUrlFactory.buildPathFromUrl();
                    if (ocdPath) {
                        GamesDataFactory.getOfferIdByPath(ocdPath).then(function (offerId) {
                            var promises = addNewCatalogData(offerId);
                            Promise.all(promises).then(fireUTagEvent);
                        });
                    }
                }else{
                    fireUTagEvent();
                }
            });
        }

        return {
            init: initUtagData,
            addNewCatalogData: addNewCatalogData,
            removeCatalogData: removeCatalogData,
            unsetCatalogInfo: unsetCatalogInfo
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function UTagDataModelFactorySingleton(LocaleFactory, AuthFactory, GamesDataFactory, ObjectHelperFactory, AppCommFactory, PdpUrlFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('UTagDataModelFactory', UTagDataModelFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.UTagDataModelFactory
     * @description
     *
     * UTagDataModelFactory
     */
    angular.module('origin-components')
        .factory('UTagDataModelFactory', UTagDataModelFactorySingleton);
}());
