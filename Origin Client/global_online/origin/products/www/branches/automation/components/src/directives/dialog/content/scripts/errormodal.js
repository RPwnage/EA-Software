
/**
 * @file dialog/content/scripts/errormodal.js
 */
(function(){
    'use strict';

    var CONTEXT_NAMESPACE = 'origin-dialog-errormodal';

    function originDialogErrormodalCtrl($scope, DialogFactory, ComponentsLogFactory, UtilFactory) {
        // used when error string is not found in dictionary
        var DEFAULT = 'default';

        // key format for error title and error body
        var KEY_FORMAT_TITLE = 'title-{errorCode}';
        var KEY_FORMAT_BODY = 'body-{errorCode}';

        /**
         * Construct localization key
         * @param  {string} keyFormat key format string - {errorCode} will be replaced with errorCode
         * @param  {string} errorCode error code
         * @return {string}           localization key
         */
        function buildKey(keyFormat, errorCode) {
            return keyFormat.replace('{errorCode}', errorCode);
        }

        /**
         * Get error strings based on error code.  Will return default
         * strings if error code is not found in dictionary.
         * @param  {string} errorCode error string identifier
         * @return {object}           error string object containing title and body strings
         */
        function getStrings(errorCode, replace) {
            errorCode = errorCode || DEFAULT;
            replace = replace || {};
            replace['{errorCode}'] = errorCode;

            function retrieveString(key, keyFormat, replace) {
                var value = UtilFactory.getLocalizedStr($scope[key], CONTEXT_NAMESPACE, key, replace);

                if (_.isString(value) && value.length === 0) {
                    key = buildKey(keyFormat, DEFAULT);
                    value = UtilFactory.getLocalizedStr($scope[key], CONTEXT_NAMESPACE, key, replace);
                }

                return value;
            }

            return {
                title: retrieveString(buildKey(KEY_FORMAT_TITLE, errorCode), KEY_FORMAT_TITLE, replace),
                body: retrieveString(buildKey(KEY_FORMAT_BODY, errorCode), KEY_FORMAT_BODY, replace)
            };
        }

        /**
         * Build error modal dialog strings (title, body, button)
         */
        function init() {
            // ObjectSerializerFactory serialize/deserialize is not suitable for this
            $scope.replace = JSON.parse(decodeURI($scope.replace || '{}'));
            $scope.strings = getStrings($scope.errorCode, $scope.replace);
            ComponentsLogFactory.error('Error Modal: ['+$scope.strings.title+'] '+$scope.strings.body);
        }

        init();
    }

    function originDialogErrormodal(ComponentsConfigFactory) {
        return {
            restrict: 'E',
            scope: {
                dialogId: '@',
                errorCode: '@',
                replace: '@',

                // error string overrides
                // TODO: this needs to be expanded with the full set of
                // checkout error code strings - body0000, title0000, etc
                titleDefault: '@',
                bodyDefault: '@',
                titleTimeout: '@',
                bodyTimeout: '@',
                titleOaxAlreadyOwnSubscription: '@',
                bodyOaxAlreadyOwnSubscription: '@',
                bodyOaxCheckoutErrorDefault: '@',
                bodyBadInput: '@',
                bodyErrorNotKnown: '@',
                titleYouownthis: '@',
                bodyYouownthis: '@'
            },
            controller: originDialogErrormodalCtrl,
            templateUrl: ComponentsConfigFactory.getTemplatePath('dialog/content/views/errormodal.html')
        };
    }

    /**
     * @ngdoc directive
     * @name origin-components.directives:originDialogErrormodal
     * @restrict E
     * @element ANY
     * @scope
     * @description
     *
     * Common error modal - accepts an error-code which is used to construct
     * the string keys - title-CODE and body-CODE - and an optional replace
     * parameter which is a URI encoded strinfigied object that contains
     * replacement parameters that will be used to complete the localized
     * string.
     *
     * @param {string} dialog-id Dialog id
     * @param {string} error-code error string identifier
     * @param {string} replace URI encoded stringified object with replacement parameters
     * @param {LocalizedText} title-default default title used when a specific message title is not found
     * @param {LocalizedText} body-default default body used when a specific message body is not found
     * @param {LocalizedText} title-timeout title for timeout message
     * @param {LocalizedText} body-timeout body for timeout message
     * @param {LocalizedText} title-oax-already-own-subscription title of purchase error when you already own a subscription
     * @param {LocalizedText} body-oax-already-own-subscription body of purchase error when you already own a subscription
     * @param {LocalizedText} body-oax-checkout-error-default body of default purchase error
     * @param {LocalizedText} body-bad-input body of purchase error when there is bad input
     * @param {LocalizedText} body-incompatible-products body of purchase error when the products cannot be purchased together
     * @param {LocalizedText} body-invalid-products body of purchase error when one or more of the products are invalid
     * @param {LocalizedText} body-error-not-known body of unknown purchase error
     * @param {LocalizedText} title-youownthis title of purchase error when attempting to buy owed product
     * @param {LocalizedText} body-youownthis body of purchase error when attempting to buy owed product
     * @param {LocalizedText} title-user-under-age Minimum Age Requirements Not Met
     * @param {LocalizedText} body-user-under-age We're sorry, but you do not meet the age requirements to make this purchase
     * @param {LocalizedText} body-4 (Checkout) There seems to be a temporary issue with your payment provider. Use a different payment method or try again later. [ref. #4]
     * @param {LocalizedText} body-10001 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10001]
     * @param {LocalizedText} body-10002 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10002]
     * @param {LocalizedText} body-10003 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10003]
     * @param {LocalizedText} body-10004 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10004]
     * @param {LocalizedText} body-10005 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10005]
     * @param {LocalizedText} body-10006 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10006]
     * @param {LocalizedText} body-10007 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10007]
     * @param {LocalizedText} body-10008 (Checkout) We're unable to process your order at this time due to an issue with your provided payment method. Please use a different payment method to complete your order, or try again later.  [ref. #10008]
     * @param {LocalizedText} body-10009 (Checkout) We're unable to process your order at this time due to an issue with your provided payment method. Please use a different payment method to complete your order, or try again later.  [ref. #10009]
     * @param {LocalizedText} body-10010 (Checkout) There seems to be a temporary issue with your payment provider. Use a different payment method or try again later. [ref. #10010]
     * @param {LocalizedText} body-10011 (Checkout) There seems to be a temporary issue with your payment provider. Use a different payment method or try again later. [ref. #10011]
     * @param {LocalizedText} body-10012 (Checkout) There seems to be a temporary issue with your payment provider. Use a different payment method or try again later. [ref. #10012]
     * @param {LocalizedText} body-10013 (Checkout) We're unable to process your order at this time due to an issue with your provided payment method. Please use a different payment method to complete your order, or try again later. [ref. #10013]
     * @param {LocalizedText} body-10014 (Checkout) We're unable to process your order at this time due to an issue with your provided payment method. Please use a different payment method to complete your order, or try again later. [ref. #10014]
     * @param {LocalizedText} body-10015 (Checkout) We're unable to process your order at this time because you did not confirm the purchase with PayPal. You will need to try again and confirm this transaction on PayPal or use a different payment method to complete your order. You will not be charged twice. [ref. #10015]
     * @param {LocalizedText} body-10022 (Checkout) There seems to be a temporary issue with your payment provider. Use a different payment method or try again later. [ref. #10022]
     * @param {LocalizedText} body-10023 (Checkout) We're unable to process your order at this time due to an issue with the billing address associated with your credit or debit card. Please check the billing address entered and make sure it matches what your card issuer or bank has on file. [ref. #10023]
     * @param {LocalizedText} body-10024 (Checkout) We're unable to process your order at this time due to an issue with your credit or debit card. Please check your card details and try again, or use a different payment method to complete your order. [ref. #10024]
     * @param {LocalizedText} body-10025 (Checkout) We're unable to process your order at this time because your credit or debit card has expired. Please use a different payment method to complete your order. [ref. #10025]
     * @param {LocalizedText} body-10033 (Checkout) We're unable to process your order at this time because you did not confirm the purchase with your payment method provider. You will need to try again and confirm this transaction on the provider website or use a different payment method to complete your order. You will not be charged twice. [ref. #10033]
     * @param {LocalizedText} body-10034 (Checkout) Your EA Wallet or MyCard account does not contain sufficient funds. Please add more funds or use a different payment method to complete this order. [ref. #10034]
     * @param {LocalizedText} body-10035 (Checkout) There seems to be a problem with your account. We're unable to process this payment. [ref. #10035]
     * @param {LocalizedText} body-10037 (Checkout) We're unable to process your payment at this time due to an issue with your phone number. Please check to make sure your number matches the correct format and try again. [ref. #10037]
     * @param {LocalizedText} body-10039 (Checkout) We're unable to process your payment. Please use another payment method or try again later. [ref. #10039]
     * @param {LocalizedText} body-10040 (Checkout) We're unable to process your order at this time due to technical issues. Please use another payment method or try again later. [ref. #10040]
     * @param {LocalizedText} body-10044 (Checkout) We were unable to process your payment. Confirm your payment method details are correct or try again later. [ref. #10044]
     * @param {LocalizedText} body-10045 (Checkout) We're unable to process your order at this time. Please use another payment method or try again later. [ref. #10045]
     * @param {LocalizedText} body-10046 (Checkout) We're unable to process your order at this time, as the payment method has been declined by your credit card issuer or bank. Please contact your bank before trying again, or use a different payment method to complete your order. [ref. #10046]
     * @param {LocalizedText} body-10047 (Checkout) We're unable to process your order at this time. Please use another payment method or try again later. [ref. #10047]
     * @param {LocalizedText} body-10048 (Checkout) We're unable to process your order at this time due to an issue with your provided payment method. Please use a different payment method to complete your order, or try again later. [ref. #10048]
     * @param {LocalizedText} body-10049 (Checkout) We're unable to process your order at this time, as the payment method has been declined by your credit card issuer or bank. Please contact your bank before trying again, or use a different payment method to complete your order. [ref. #10049]
     * @param {LocalizedText} body-10050 (Checkout) We're unable to process your order at this time due to an issue with your provided payment method. Please use a different payment method to complete your order, or try again later. [ref. #10050]
     * @param {LocalizedText} body-10051 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10051]
     * @param {LocalizedText} body-10052 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10052]
     * @param {LocalizedText} body-10053 (Checkout) That didn't work, let's try again.  If the problem persists, wait a few minutes to see if the technical issue is resolved. [ref. #10053]
     * @param {LocalizedText} body-10054 (Checkout) We're unable to process your order at this time. Please use another payment method or try again later. [ref. #10054]
     * @param {LocalizedText} body-10055 (Checkout) Your pre-paid card does not have enough funds to complete this transaction. Please use a different payment option. [ref. #10055]
     * @param {LocalizedText} body-10056 (Checkout) We're unable to process your order at this time. Please use another payment method or try again later. [ref. #10056]
     * @param {LocalizedText} body-50000 (Checkout) Your order could not be completed. Please try again later. [ref. #50000]
     * @param {LocalizedText} body-50001 (Checkout) Your order could not be completed. Please try again later. [ref. #50001]
     * @param {LocalizedText} body-51000 (Checkout) Your order could not be completed. Please try again later. [ref. #51000]
     * @param {LocalizedText} body-51001 (Checkout) Your order could not be completed. Please try again later. [ref. #51001]
     * @param {LocalizedText} body-51002 (Checkout) Your order could not be completed. Please try again later. [ref. #51002]
     * @param {LocalizedText} body-51003 (Checkout) Your order could not be completed. Please try again later. [ref. #51003]
     * @param {LocalizedText} body-51004 (Checkout) Your order could not be completed because you haven't selected items to purchase. [ref. #51004]
     * @param {LocalizedText} body-51005 (Checkout) Your order could not be completed. Please try again later. [ref. #51005]
     * @param {LocalizedText} body-51006 (Checkout) Your order could not be completed. Please try again later. [ref. #51006]
     * @param {LocalizedText} body-51007 (Checkout) Your order could not be completed. Please try again later. [ref. #51007]
     * @param {LocalizedText} body-51008 (Checkout) Your order could not be completed. Please try again later. [ref. #51008]
     * @param {LocalizedText} body-51009 (Checkout) Your order could not be completed. Please try again later. [ref. #51009]
     * @param {LocalizedText} body-51010 (Checkout) Your order could not be completed. Please try again later. [ref. #51010]
     * @param {LocalizedText} body-51011 (Checkout) Your order could not be completed. Please try again later. [ref. #51011]
     * @param {LocalizedText} body-51012 (Checkout) Your order could not be completed. Please try again later. [ref. #51012]
     * @param {LocalizedText} body-51013 (Checkout) Your order could not be completed. Please try again later. [ref. #51013]
     * @param {LocalizedText} body-51014 (Checkout) Your order could not be completed. Please try again later. [ref. #51014]
     * @param {LocalizedText} body-53000 (Checkout) There was an issue accessing your account information. Sign in and place your order again.  [ref. #53000]
     * @param {LocalizedText} body-53001 (Checkout) There was an issue accessing your account information. Sign in and place your order again.  [ref. #53001]
     * @param {LocalizedText} body-53002 (Checkout) There was an issue accessing your account information. Sign in and place your order again.  [ref. #53002]
     * @param {LocalizedText} body-53003 (Checkout) There was an issue accessing your account information. Sign in and place your order again.  [ref. #53003]
     * @param {LocalizedText} body-54000 (Checkout) We're unable to process your order right now. Use a different payment method or try again later.  [ref. #54000]
     * @param {LocalizedText} body-54001 (Checkout) That didn't work, let's try again. If the problem persists you may want to use a different payment method or try again later.  [ref. #54001]
     * @param {LocalizedText} body-54003 (Checkout) There are item(s) in your cart that are no longer available. Please remove them and try placing your order again.  [ref. #54003]
     * @param {LocalizedText} body-54005 (Checkout) We're unable to process your order right now. Use a different payment method or try again later. [ref. #54005]
     * @param {LocalizedText} body-54006 (Checkout) Sorry - you don't meet the age requirement to make this purchase. [ref. #54006]
     * @param {LocalizedText} body-54101 (Checkout) Your EA Wallet does not contain sufficient funds. Please add more funds or use a different payment method to complete this order. [ref. #54101]
     * @param {LocalizedText} body-57000 (Checkout) Sorry, there seems to be a problem with your account. Please sign in and try again later.
     * @param {LocalizedText} body-57003 (Checkout) Your membership is not currently active. Please log into your EA Account to view and change your  account status.
     * @param {LocalizedText} body-57103 (Checkout) Only one free trial is allowed per payment method. You'll need to use a different payment method to sign up for this free trial.
     * @param {LocalizedText} body-57105 (Checkout) Sorry - you've already completed a free trial on this EA Account.
     * @param {LocalizedText} body-57106 (Checkout) You're already an active Origin Access member - no need to sign up twice!
     * @param {LocalizedText} body-58000 (Checkout) Your order could not be completed. Try again.  [ref. #58000]
     * @param {LocalizedText} body-58001 (Checkout) Your order could not be completed. Try again. [ref. #58001]
     * @param {LocalizedText} body-58003 (Checkout) Your order could not be completed. Try again. [ref. #58003]
     * @param {LocalizedText} body-58005 (Checkout) This order has already been submitted. You will not be charged twice. [ref. #58005]
     * @param {LocalizedText} body-58006 (Checkout) Sorry, this payment method is not currently supported. [ref. #58006]
     * @param {LocalizedText} body-58009 (Checkout) There seems to be a temporary issue with your payment provider. Use a different payment method or try again later. [ref. #58009]
     * @param {LocalizedText} body-56001 (Checkout Gifting) Recipient not found or invalid status(banned, suspended) [ref. #56001]
     * @param {LocalizedText} body-56002 (Checkout Gifting) Non-giftable offer found in cart [ref. #56002]
     * @param {LocalizedText} body-56003 (Checkout Gifting) Gift cannot be sent between different storefronts [ref. #56003]
     * @param {LocalizedText} body-56004 (Checkout Gifting) Recipient already owns the game [ref. #56004]
     * @param {LocalizedText} title-936000 (Checkout) Minimum Age Requirements Not Met [ref. #936000]
     * @param {LocalizedText} body-936000 (Checkout) We're sorry, but you do not meet the age requirements to make this purchase [ref. #936000]
     * @param {LocalizedText} body-51015 (Checkout) Your browser is blocking third party cookies, which prevents checkout from loading. Please enable third part cookies and try again. [ref. #51015]
     * @param {LocalizedText} title-7105 (Subs Checkout) User has already purchased subscriptions trial - title
     * @param {LocalizedText} body-7105 (Subs Checkout) User has already purchased subscriptions trial - body
     * @param {LocalizedText} title-7103 (Subs Checkout) Payment method aready used to purchase subscriptions trial - title
     * @param {LocalizedText} body-7103 (Subs Checkout) Payment method aready used to purchase subscriptions trial - body
     *
     * @example
     * <example module="origin-components">
     *     <file name="index.html">
     *     <origin-dialog-errormodal dialog-id="error-modal-dialog" error-code="5000" />
     *     </file>
     * </example>
     */
    angular.module('origin-components')
        .controller('originDialogErrormodalCtrl', originDialogErrormodalCtrl)
        .directive('originDialogErrormodal', originDialogErrormodal);
}());
