/*jshint unused: false */
/*jshint strict: false */
define([], function() {

    /**
     * user related data
     * @module module:user
     * @memberof module:Experiment
     */

    var VALUE_CLEARED = '';

    var DATA_INIT = {
            originId: '',
            personaId: '',
            userPID: '',
            underAge: false,
            dob: '',
            email: '',
            emailStatus: '',
            emailSignup: false,
            locale: 'en_us',
            storeFront: 'usa',
            subscriber: false,
            subscriberInfo: {},
            entitlements: [],
            extraContentEntitlements: []
        },
        data = DATA_INIT;


    function getPersonaId() {
        return data.personaId;
    }

    function getUserPid() {
        return data.userPID;
    }

    function getDob() {
        return data.dob;
    }

    function getUserGlobalEmailStatus() {
        return data.emailSignup;
    }

    function getUnderage() {
        return data.underAge;
    }

    function init(userObj) {
        data = userObj;
    }

    function getUserObj() {
        return data;
    }

    function clearUserObj () {
        init(DATA_INIT);
    }

    function setLocale(locale) {
        data.locale = locale;
    }

    function setStoreFront(storefront) {
        data.storeFront = storefront;
    }

    return { /** @lends module:Experiment:module:user */

        /**
         * initializes experiment user object
         * @param {object} userObj
         * @method
         */
        init: init,

        /**
         * return logged in user's personaId
         * @return {string}
         * @method
         */
        personaId: getPersonaId,

        /**
         * returns logged in user's nucleus Id
         * @return {string}
         * @method
         */
        userPid: getUserPid,

        /**
         * returns whether logged in user's underAge or not
         * @return {boolean}
         * @method
         */
        underAge: getUnderage,

        /**
         * returns the logged in user's date of birth
         * @return {string}
         * @method
         */
        dob: getDob,

        /**
         * returns the logged in user's global Origin email sign up status
         * @return {string}
         * @method
         */
        globalEmailSignup: getUserGlobalEmailStatus,

        /**
         * sets the user's locale
         * @method
         */
        setLocale: setLocale,

        /**
         * sets the user's storeFront
         * @method
         */
        setStoreFront: setStoreFront,

        /**
         * returns user data obj
         * @method
         * @return {Object}
         */
        getUserObj: getUserObj,

        /**
         * clears user data obj
         * @method
         */
        clearUserObj: clearUserObj
    };
});