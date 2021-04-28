/**
 * @file models/gamescommerce.js
 */
(function() {
    'use strict';

    function GamesCommerceFactory($state, ComponentsLogFactory, ObjectHelperFactory, GamesCatalogFactory, GamesEntitlementFactory) {
        var myEvents = new Origin.utils.Communicator(),
            DEFAULT_COMMERCE_PROFILE = 'oig-real',
            DEFAULT_PLATFORM = 'PCWIN';

        function handleGetOdcProfileError(error) {
            ComponentsLogFactory.error('[GamesCommerceFactory] ODC profile retrieval failed:', error);
            return {};
        }

        /**
         * @typedef checkoutMessageObject
         * @type {object}
         * @property {string} stripetitlestr the text to show in the stripe title
         * @property {string} stripebodystr the text to show in the stripe body
         * @property {string} titlestr the text to show in the modal header
         */

        /**
         * @typedef odcProfileObject
         * @type {object}
         * @property {string} odcProfile the ODC profile ID
         * @property {string} categoryId the category ID to use during Lockbox topup flow checkout
         * @property {string} fictionalCurrencyCode the fictional currency code, i.e. '_BW' or '_FF'
         * @property {string} invoiceSource the desired invoiceSource
         * @property {checkoutMessageObject} checkoutMessage the optional checkout message parameters that appear in the addon store stripe
         * @property {[string]} oigOfferBlacklist the optional set of offers that should not be displayed in the OIG addon store
         */

        /**
         * Builds an ODC profile data model object based on the server response data
         *
         * @param {Object} response The server response data
         * @param {string} odcProfile the ODC profile ID that was queried
         * @return {odcProfileObject} the processed ODC profile data model
         */
        function handleOdcProfileResponse(response, odcProfile) {
            return {
                'odcProfile': odcProfile,
                'categoryId': _.get(response, ['properties', 'categoryId']) || '',
                'fictionalCurrencyCode': _.get(response, ['properties', 'fictionalCurrencyCode']) || '',
                'invoiceSource': _.get(response, ['properties', 'invoiceSource']) || '',
                'checkoutMessage': _.get(response, ['properties', 'checkoutMessage']) || {},
                'oigOfferBlacklist': _.get(response, ['properties', 'oigOfferBlacklist']) || [],
            };
        }

        /**
         * Retrieves profile ID for the given offer
         *
         * @param offerId The offer ID
         * @return {string} The profile ID
         * @throws {Error} If base game offer ID could not be found
         */
        function getProfileIdFromOffer(offerId) {
            if (!offerId) {
                throw new Error('[GamesCommerceFactory] Could not find a base game offer ID for this content.');
            }

            return GamesCatalogFactory.getCatalogInfo([offerId])
                .then(ObjectHelperFactory.getProperty(offerId))
                .then(extractProfile);
        }

        /**
         * Extracts the ODC profile ID from the given catalog data.
         *
         * @param {Object} catalogInfo The catalog data model for an offer
         * @return {string} ODC profile ID
         */
        function extractProfile(catalogInfo) {
            var currentPlatform = Origin.utils.os(),
                commerceProfile = _.get(catalogInfo, ['platforms', currentPlatform, 'commerceProfile']);

            if (!commerceProfile && currentPlatform !== DEFAULT_PLATFORM) {
                commerceProfile = _.get(catalogInfo, ['platforms', DEFAULT_PLATFORM, 'commerceProfile']);
            }

            return commerceProfile ? commerceProfile : '';
        }

        /**
         * Gets an offer ID for a base game within the given masterTitleId
         *
         * @param {[string]} masterTitleIds A list of master title IDs configured for this game.
         * @return {Promise<string>} Promise that resolves into the base game offer ID
         */
        function getBaseGameOfferId(masterTitleIds) {
            var baseGameEntitlement,
                promises = [],
                catalog = GamesCatalogFactory.getCatalog();

            _.forEach(masterTitleIds, function(masterTitleId) {
                baseGameEntitlement = GamesEntitlementFactory.getBaseGameEntitlementByMasterTitleId(masterTitleId, catalog);
                if (baseGameEntitlement) {
                    return false; // Break the _.forEach loop
                }
            });

            if (baseGameEntitlement) {
                return Promise.resolve(baseGameEntitlement.offerId);
            }

            _.forEach(masterTitleIds, function(masterTitleId) {
                promises.push(GamesCatalogFactory.getPurchasableOfferIdByMasterTitleId(masterTitleId, true));
            });

            return Promise.all(promises)
                .then(extractValidOfferId);
        }

        /**
         * Extracts the first valid offer ID from the passed array
         *
         * @param {[string]} offerIds Array of offer IDs.  Some null/undefined/empty values are expected.
         * @return {string} The first valid offerId in the array
         */
        function extractValidOfferId(offerIds) {
            var validOfferId;
            _.forEach(offerIds, function(offerId) {
                if (offerId) {
                    validOfferId = offerId;
                    return false; // Break the _.forEach loop
                }
            });
            return validOfferId;
        }

        /**
         * Gets the base game ODC profile ID in the event that the current offer is a DLC and is not configured with an ODC profile ID.
         *
         * @param {Object} catalogInfo The catalog data model for an offer
         * @return {Promise<string>} Promise that resolves into a ODC profile ID
         */
        function getBaseGameProfileIfNecessary(catalogInfo) {
            var odcProfile,
                masterTitleIds;

            if (!catalogInfo) {
                ComponentsLogFactory.error('[GamesCommerceFactory] Catalog lookup failed.');
                return '';
            }

            odcProfile = extractProfile(catalogInfo);
            if (odcProfile) {
                return Promise.resolve(odcProfile);
            }

            // If the DLC did not have a odcProfile set, look up the parent's odcProfile (if a parent exists).
            masterTitleIds = catalogInfo.alternateMasterTitleIds.concat(catalogInfo.masterTitleId);
            return getBaseGameOfferId(masterTitleIds)
                .then(getProfileIdFromOffer);
        }

        /**
         * Ensures that the given ODC profile ID is valid.  If not, returns a default value.
         *
         * @param {string} odcProfile The ODC profile ID to validate
         * @return {string} The ODC profile ID
         */
        function validateProfile(odcProfile) {
            return Promise.resolve(odcProfile || DEFAULT_COMMERCE_PROFILE);
        }

        /**
         * Fetches and processes ODC profile data from CMS
         *
         * @param {string} odcProfile The ODC profile ID to fetch
         * @return {Promise<odcProfileObject>} Promise that resolves into ODC profile data
         */
        function getOdcProfileById(odcProfile) {
            return Origin.games.getOdcProfile(odcProfile, Origin.locale.languageCode())
                .then(_.partialRight(handleOdcProfileResponse, odcProfile));
        }

        /**
         * Shows the addon store with a custom checkout message if one exists in the commerce profile.
         *
         * @param {string} id The ODC profile ID
         * @param {string} masterTitleId The master title ID of the addon store to show if there is a checkout message.
         * @param {odcProfileObject} profile The profile data that may contain a custom checkout message.
         * @return {Promise} Promise that rejects if a custom checkout message exists.
         */
        function showCheckoutMessage(id, masterTitleId, profile) {
            // If profile is configured to show a checkout message, show it.
            if (!_.isEmpty(profile.checkoutMessage)) {
                $state.go('app.oig-addon-store', {
                    profile: id,
                    masterTitleId: masterTitleId
                });
                return Promise.reject();
            }
            return Promise.resolve();
        }

        /**
         * Retrieves ODC profile data for the given offer.
         *
         * @param {string} offerId The offer for which to retrieve ODC profile data
         * @return {Promise<odcProfileObject>} Promise that resolves into an ODC profile data model object
         */
        function getOdcProfile(offerId) {
            return GamesCatalogFactory.getCatalogInfo([offerId])
                .then(ObjectHelperFactory.getProperty(offerId))
                .then(getBaseGameProfileIfNecessary)
                .then(validateProfile)
                .then(getOdcProfileById)
                .catch(handleGetOdcProfileError);
        }

        /**
         * Retrieves ODC profile data for the given profile ID and checks for the presence of a checkout message.
         *
         * @param {string} id The profile ID for which to retrieve ODC profile data
         * @param {string} masterTitleId The master title ID of the addon store to show if there is a checkout message.
         * @return {Promise} response Promise that rejects if a checkout message exists
         */
        function checkForCheckoutMessage(id, masterTitleId) {
            return validateProfile(id)
                .then(getOdcProfileById)
                .catch(handleGetOdcProfileError)
                .then(_.partial(showCheckoutMessage, id, masterTitleId));
        }

        function getWalletBalance(currencyCode) {
            return Origin.commerce.getWalletBalance(currencyCode);
        }

        function queryCheckout(profile, offerId) {
            return Origin.commerce.postVcCheckout([offerId], profile.fictionalCurrencyCode, profile.odcProfile);
        }

        function postVcCheckout(offerId) {
            return getOdcProfile(offerId)
                .then(_.partialRight(queryCheckout, offerId));
        }

        return {
            events: myEvents,

            /**
             * Retrieves ODC profile data for the given offer.
             *
             * @param {string} offerId The offer for which to retrieve ODC profile data
             * @return {Promise<odcProfileObject>} response Promise that resolves into a ODC profile data model object
             * @method getOdcProfile
             */
            getOdcProfile: getOdcProfile,

            /**
             * Retrieves ODC profile data for the given profile ID.
             *
             * @param {string} id The ODC profile ID for which to retrieve ODC profile data
             * @return {Promise<odcProfileObject>} response Promise that resolves into a ODC profile data model object
             * @method getOdcProfileById
             */
            getOdcProfileById: getOdcProfileById,

            /**
             * Retrieves ODC profile data for the given profile ID and checks for the presence of a checkout message.
             * If a message exists, this function will reject the promise and navigate to the addon store.
             *
             * @param {string} id The profile ID for which to retrieve ODC profile data
             * @param {string} masterTitleId The master title ID of the addon store to show if there is a checkout message.
             * @return {Promise} response Promise that rejects if a checkout message exists
             * @method checkForCheckoutMessage
             */
            checkForCheckoutMessage: checkForCheckoutMessage,

            /**
             * Retrieves the current balance for the given currency.
             * @param  {string} currency The currency code, i.e. '_BW', '_FF'
             * @return {promise<number>} response A promise that resolves into the current balance for the given currency
             * @method getWalletBalance
             */
            getWalletBalance: getWalletBalance,

            /**
             * Entitles the user to the given offer if the user wallet has sufficient currency
             * @param  {string} offerId The offer ID to purchase
             * @return {promise<string>} response A promise that resolves into an order number
             * @method postVcCheckout
             */
            postVcCheckout: postVcCheckout
        };
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GamesCommerceFactory

     * @description
     *
     * GamesCommerceFactory
     */
    angular.module('origin-components')
        .factory('GamesCommerceFactory', GamesCommerceFactory);
}());