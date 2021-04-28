/*jshint unused: false */
/*jshint strict: false */
define([], function() {

    /**
     * user related data
     * @module module:user
     * @memberof module:Origin
     */

    var VALUE_CLEARED = '';

    var data = {
        auth: {
            accessToken: ''
        },
        originId: '',
        personaId: '',
        userPID: '',
        underAge: false,
        userStatus: '',
        dob: '',
        email: '',
        emailStatus: '',
        emailSignup: false,
        showPersona: '',
        tfaSignup: false,
        registrationDate: ''
    };

    function setAccessToken(token) {
        data.auth.accessToken = token;
    }

    function setAccessTokenExpireDate(expireDate) {
        data.auth.expireDate = expireDate;
    }

    function getAccessToken() {
        return data.auth.accessToken;
    }

    function isAccessTokenExpired() {
        return Date.now() > data.auth.expireDate;
    }

    function setOriginId(originId) {
        data.originId = originId;
    }

    function setUserStatus(userStatus) {
        data.userStatus = userStatus;
    }

    function getOriginId() {
        return data.originId;
    }

    function setUnderAge(flag) {
        data.underAge = flag;
    }

    function setPersonaId(personaId) {
        data.personaId = personaId;
    }

    function getPersonaId() {
        return data.personaId;
    }

    function setUserPid(userId) {
        data.userPID = userId;
    }

    function getUserPid() {
        return data.userPID;
    }

    function setShowPersona(showPersona) {
        data.showPersona = showPersona;
    }

    function getUserStatus() {
        return data.userStatus;
    }

    function getShowPersona() {
        return data.showPersona;
    }

    function setDob(dob) {
        data.dob = dob;
    }

    function getDob() {
        return data.dob;
    }

    function setUserEmail(email) {
        data.email = email;
    }

    function getUserEmail() {
        return data.email;
    }

    function setUserEmailVerifyStatus(status) {
        data.emailStatus = status;
    }

    function getUserEmailVerifyStatus() {
        return data.emailStatus;
    }

    function setUserGlobalEmailStatus(status) {
        data.emailSignup = (status === 'true');
    }

    function getUserGlobalEmailStatus() {
        return data.emailSignup;
    }

    function setTFAStatus(status) {
        data.tfaSignup = status;
    }

    function getTFAStatus() {
        return data.tfaSignup;
    }

    function setRegistrationDate(registrationDate) {
        data.registrationDate = registrationDate;
    }

    function getRegistrationDate() {
        return data.registrationDate;
    }

    function clearUserGlobalEmailStatus() {
        data.emailSignup = false;
    }

    function clearUserEmail() {
        data.email = VALUE_CLEARED;
    }

    function clearUserEmailStatus() {
        data.emailStatus = VALUE_CLEARED;
    }

    function clearAccessToken() {
        data.auth.accessToken = VALUE_CLEARED;
    }

    function clearUserStatus() {
        data.userStatus = VALUE_CLEARED;
    }

    function clearAccessTokenExpireDate() {
        data.auth.expireDate = VALUE_CLEARED;
    }

    function clearOriginId() {
        data.originId = VALUE_CLEARED;
    }

    function clearPersonaId() {
        data.personaId = VALUE_CLEARED;
    }

    function clearShowPersona() {
        data.showPersona = VALUE_CLEARED;
    }

    function clearUserPid() {
        data.userPID = VALUE_CLEARED;
    }

    function clearTFAStatus() {
        data.tfaSignup = false;
    }

    function clearRegistrationDate() {
        data.registrationDate = VALUE_CLEARED;
    }

    function clearUserAuthInfo() {
        clearAccessToken();
        clearUserPid();
        clearOriginId();
        clearPersonaId();
        clearShowPersona();
        clearUserEmail();
        clearUserEmailStatus();
        clearUserGlobalEmailStatus();
        clearTFAStatus();
        clearRegistrationDate();
        clearUserStatus();
    }

    function getUnderage() {
        return data.underAge;
    }

    return {

        //These are exposed by the JSSDK
        publicObjs: /** @lends module:Origin.module:user */{
            /**
             * returns the JSSDK access_token
             * @return {string}
             * @method
             */
            accessToken: getAccessToken,

            /**
             * returns the true if the access token is expired based off of the expires_in property in response
             * @return {boolean}
             * @method
             */
            isAccessTokenExpired: isAccessTokenExpired,

            /**
             * return logged in user's originId
             * @return {string}
             * @method
             */
            originId: getOriginId,

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
             * Returns user's status: ACTIVE/BANNED/etc
             */
            userStatus: getUserStatus,

            /**
             * returns the logged in user's date of birth
             * @return {string}
             * @method
             */
            dob: getDob,

            /**
             * returns the logged in user's email address
             * @return {string}
             * @method
             */
            email: getUserEmail,

            /**
             * returns the logged in user's email verification status
             * @return {string}
             * @method
             */
            emailStatus: getUserEmailVerifyStatus,

            /**
             * returns the logged in user's global Origin email sign up status
             * @return {string}
             * @method
             */
            globalEmailSignup: getUserGlobalEmailStatus,

            /**
             * returns the logged in user's showPersona setting
             * @type {string}
             * @method
             */
            showPersona: getShowPersona,

            /**
             * returns whether logged in user has signed up for TFA or not
             * @return {boolean}
             * @method
             */
            tfaSignup: getTFAStatus,

            /**
             * returns date that logged in user registered their Origin account
             * @return {boolean}
             * @method
             */
            registrationDate: getRegistrationDate
        },

        //These are not exposed by the JSSDK, are used by auth to manage userInfo's internal data.
        setOriginId: setOriginId,
        setAccessToken: setAccessToken,
        setAccessTokenExpireDate: setAccessTokenExpireDate,
        setUnderAge: setUnderAge,
        setPersonaId: setPersonaId,
        setUserPid: setUserPid,
        setUserStatus: setUserStatus,
        setShowPersona: setShowPersona,
        setDob: setDob,
        setUserEmail: setUserEmail,
        setUserEmailVerifyStatus: setUserEmailVerifyStatus,
        setUserGlobalEmailStatus: setUserGlobalEmailStatus,
        setTFAStatus: setTFAStatus,
        setRegistrationDate: setRegistrationDate,

        clearPersonaId: clearPersonaId,
        clearUserPid: clearUserPid,
        clearUserStatus: clearUserStatus,
        clearShowPersona: clearShowPersona,
        clearUserAuthInfo: clearUserAuthInfo,
        clearOriginId: clearOriginId,
        clearAccessToken: clearAccessToken,
        clearUserEmail: clearUserEmail,
        clearUserEmailStatus: clearUserEmailStatus,
        clearUserGlobalEmailStatus: clearUserGlobalEmailStatus,
        clearAccessTokenExpireDate: clearAccessTokenExpireDate,
        clearTFAStatus: clearTFAStatus,
        clearRegistrationDate: clearRegistrationDate
    };


});