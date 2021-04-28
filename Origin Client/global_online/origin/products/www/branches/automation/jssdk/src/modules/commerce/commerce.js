/*jshint unused: false */
/*jshint strict: false */
define([
    'promise',
    'core/dataManager',
    'core/user',
    'core/urls',
    'core/errorhandler',
], function(Promise, dataManager, user, urls, errorhandler) {

    /**
     * @module module:commerce
     * @memberof module:Origin
     */

    function handleWalletBalanceResponse(response) {
        var i, length;
        if (response.billingaccounts) {
            length = response.billingaccounts.length;
            for (i = 0; i < length; i++) {
                if (response.billingaccounts[i].status === 'ACTIVE') {
                    return Number(response.billingaccounts[i].balance);
                }
            }
        }
        return 0;
    }

    function getWalletBalance(currency) {
        var endPoint = urls.endPoints.walletBalance;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'Accept',
                'val': 'application/vnd.origin.v2+json; x-cache/force-write'
            }],
            parameters: [{
                'label': 'userId',
                'val': user.publicObjs.userPid()
            }, {
                'label': 'currency',
                'val': currency
            }],
            appendparams: [],
            reqauth: true,
            requser: true,
            responseHeader: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0)
            .then(handleWalletBalanceResponse, errorhandler.logAndCleanup('COMMERCE:getWalletBalance FAILED'));
    }

    function handleVcCheckoutResponse(response) {
        if (response.invoiceNumbers && response.invoiceNumbers.length > 0) {
            return response.invoiceNumbers[0];
        } else {
            return errorhandler.promiseReject('VC Checkout failed to grant entitlement(s)');
        }
    }

    function postVcCheckout(offerIds, currency, odcProfile) {
        var endPoint = urls.endPoints.vcCheckout;
        var config = {
            atype: 'POST',
            headers: [{
                'label': 'Accept',
                'val': 'application/vnd.origin.v3+json; x-cache/force-write'
            }, {
                'label': 'Content-Type',
                'val': 'application/json'
            }],
            parameters: [{
                'label': 'userId',
                'val': user.publicObjs.userPid()
            }, {
                'label': 'currency',
                'val': currency
            }, {
                'label': 'profile',
                'val': odcProfile
            }],
            appendparams: [],
            reqauth: true,
            requser: true,
            responseHeader: false
        };

        var body = {
            checkout: {
                lineItems: []
            }
        };

        var numOffers = offerIds.length;
        for (var i = 0; i < numOffers; i++) {
            body.checkout.lineItems.push({
                productId: offerIds[i],
                quantity: 1
            });
        }
        config.body = JSON.stringify(body);

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0)
            .then(handleVcCheckoutResponse, errorhandler.logAndCleanup('COMMERCE:postVcCheckout FAILED'));
    }

    return/** @lends module:Origin.module:commerce */ {

        /**
         * Retrieves the current balance for the given currency.
         * @param  {string} currency The currency code, i.e. '_BW', '_FF'
         * @return {promise<number>} response A promise that resolves into the current balance for the given currency
         * @method getWalletBalance
         */
        getWalletBalance: getWalletBalance,

        /**
         * Entitles the user to the given offers if the user wallet has sufficient currency.
         * @param  {string[]} offerIds The list of offer IDs to purchase
         * @param  {string} currency The currency code, i.e. '_BW', '_FF'
         * @param  {string} odcProfile The ODC profile ID associated with the given offers
         * @return {promise<string>} response A promise that resolves into the order number
         * @method postVcCheckout
         */
        postVcCheckout: postVcCheckout
    };
});