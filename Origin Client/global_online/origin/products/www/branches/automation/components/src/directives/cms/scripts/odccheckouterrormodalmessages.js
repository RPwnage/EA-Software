/**
 * @file cms/odccheckouterrormodalmessages.js
 */
(function(){
    'use strict';

    /**
     * @ngdoc directive
     * @name origin-components.directives:odcCheckoutErrorModalMessages
     * @restrict E
     * @element ANY
     * @scope
     * @param {LocalizedString} title-default - Error modal title body
     * @param {LocalizedString} body-default - Default error body
     * @param {LocalizedString} title-timeout - Error modal title for timeout error
     * @param {LocalizedString} body-timeout - Error modal body for timeout error
     * @param {LocalizedString} title-login-required - Login Required
     * @param {LocalizedString} body-login-required - You must login/register to continue.
     * @param {LocalizedString} title-oax-already-own-subscription - Error modal title for already own subscription error
     * @param {LocalizedString} body-oax-already-own-subscription - Error modal body for already own subscription error
     * @param {LocalizedString} body-oax-checkout-error-default - Default error modal body for oax
     * @param {LocalizedString} body-bad-input - Error modal body for bad input error
     * @param {LocalizedString} body-error-not-known - Error modal body for unknown error
     * @param {LocalizedString} title-youownthis - Error modal title for trying to entitle an offer already owned
     * @param {LocalizedString} body-youownthis - Error modal body for trying to entitle an offer already owned
     * @param {LocalizedString} title-user-under-age - Error modal title for trying to entitle when the account doesn't meet the age restrictions of the offer
     * @param {LocalizedString} body-user-under-age - Error modal body for trying to entitle when the account doesn't meet the age restrictions of the offer
     * @param {LocalizedString} body-4 - The body text to display for error 4
     * @param {LocalizedString} body-10001 - The body text to display for error 10001
     * @param {LocalizedString} body-10002 - The body text to display for error 10002
     * @param {LocalizedString} body-10003 - The body text to display for error 10003
     * @param {LocalizedString} body-10004 - The body text to display for error 10004
     * @param {LocalizedString} body-10005 - The body text to display for error 10005
     * @param {LocalizedString} body-10006 - The body text to display for error 10006
     * @param {LocalizedString} body-10007 - The body text to display for error 10007
     * @param {LocalizedString} body-10008 - The body text to display for error 10008
     * @param {LocalizedString} body-10009 - The body text to display for error 10009
     * @param {LocalizedString} body-10010 - The body text to display for error 10010
     * @param {LocalizedString} body-10011 - The body text to display for error 10011
     * @param {LocalizedString} body-10012 - The body text to display for error 10012
     * @param {LocalizedString} body-10013 - The body text to display for error 10013
     * @param {LocalizedString} body-10014 - The body text to display for error 10014
     * @param {LocalizedString} body-10015 - The body text to display for error 10015
     * @param {LocalizedString} body-10022 - The body text to display for error 10022
     * @param {LocalizedString} body-10023 - The body text to display for error 10023
     * @param {LocalizedString} body-10024 - The body text to display for error 10024
     * @param {LocalizedString} body-10025 - The body text to display for error 10025
     * @param {LocalizedString} body-10033 - The body text to display for error 10033
     * @param {LocalizedString} body-10034 - The body text to display for error 10034
     * @param {LocalizedString} body-10035 - The body text to display for error 10035
     * @param {LocalizedString} body-10037 - The body text to display for error 10037
     * @param {LocalizedString} body-10039 - The body text to display for error 10039
     * @param {LocalizedString} body-10040 - The body text to display for error 10040
     * @param {LocalizedString} body-10044 - The body text to display for error 10044
     * @param {LocalizedString} body-10045 - The body text to display for error 10045
     * @param {LocalizedString} body-10046 - The body text to display for error 10046
     * @param {LocalizedString} body-10047 - The body text to display for error 10047
     * @param {LocalizedString} body-10048 - The body text to display for error 10048
     * @param {LocalizedString} body-10049 - The body text to display for error 10049
     * @param {LocalizedString} body-10050 - The body text to display for error 10050
     * @param {LocalizedString} body-10051 - The body text to display for error 10051
     * @param {LocalizedString} body-10052 - The body text to display for error 10052
     * @param {LocalizedString} body-10053 - The body text to display for error 10053
     * @param {LocalizedString} body-10054 - The body text to display for error 10054
     * @param {LocalizedString} body-10055 - The body text to display for error 10055
     * @param {LocalizedString} body-10056 - The body text to display for error 10056
     * @param {LocalizedString} body-50000 - The body text to display for error 50000
     * @param {LocalizedString} body-50001 - The body text to display for error 50001
     * @param {LocalizedString} body-51000 - The body text to display for error 51000
     * @param {LocalizedString} body-51001 - The body text to display for error 51001
     * @param {LocalizedString} body-51002 - The body text to display for error 51002
     * @param {LocalizedString} body-51003 - The body text to display for error 51003
     * @param {LocalizedString} body-51004 - The body text to display for error 51004
     * @param {LocalizedString} body-51005 - The body text to display for error 51005
     * @param {LocalizedString} body-51006 - The body text to display for error 51006
     * @param {LocalizedString} body-51007 - The body text to display for error 51007
     * @param {LocalizedString} body-51008 - The body text to display for error 51008
     * @param {LocalizedString} body-51009 - The body text to display for error 51009
     * @param {LocalizedString} body-51010 - The body text to display for error 51010
     * @param {LocalizedString} body-51011 - The body text to display for error 51011
     * @param {LocalizedString} body-51012 - The body text to display for error 51012
     * @param {LocalizedString} body-51013 - The body text to display for error 51013
     * @param {LocalizedString} body-51014 - The body text to display for error 51014
     * @param {LocalizedString} body-53000 - The body text to display for error 53000
     * @param {LocalizedString} body-53001 - The body text to display for error 53001
     * @param {LocalizedString} body-53002 - The body text to display for error 53002
     * @param {LocalizedString} body-53003 - The body text to display for error 53003
     * @param {LocalizedString} body-54000 - The body text to display for error 54000
     * @param {LocalizedString} body-54001 - The body text to display for error 54001
     * @param {LocalizedString} body-54003 - The body text to display for error 54003
     * @param {LocalizedString} body-54005 - The body text to display for error 54005
     * @param {LocalizedString} body-54006 - The body text to display for error 54006
     * @param {LocalizedString} body-54101 - The body text to display for error 54101
     * @param {LocalizedString} body-57000 - The body text to display for error 57000
     * @param {LocalizedString} body-57003 - The body text to display for error 57003
     * @param {LocalizedString} body-57103 - The body text to display for error 57103
     * @param {LocalizedString} body-57105 - The body text to display for error 57105
     * @param {LocalizedString} body-57106 - The body text to display for error 57106
     * @param {LocalizedString} body-58000 - The body text to display for error 58000
     * @param {LocalizedString} body-58001 - The body text to display for error 58001
     * @param {LocalizedString} body-58003 - The body text to display for error 58003
     * @param {LocalizedString} body-58005 - The body text to display for error 58005
     * @param {LocalizedString} body-58006 - The body text to display for error 58006
     * @param {LocalizedString} body-58009 - The body text to display for error 58009
     *
     * @description For Odc Checkout Error Modal
     *
     * 
     */
}());

