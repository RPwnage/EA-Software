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
        dob: ''
    };

    function setAccessToken(token) {
        data.auth.accessToken = token;
    }

    function getAccessToken() {
        return data.auth.accessToken;
    }

    function setOriginId(originId) {
        data.originId = originId;
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

    function setDob(dob) {
        data.dob = dob;
    }

    function getDob() {
        return data.dob;
    }

    function clearAccessToken() {
        data.auth.accessToken = VALUE_CLEARED;
    }

    function clearOriginId() {
        data.originId = VALUE_CLEARED;
    }

    function clearPersonaId() {
        data.personaId = VALUE_CLEARED;
    }

    function clearUserPid() {
        data.userPID = VALUE_CLEARED;
    }

    function clearUserAuthInfo() {
        clearAccessToken();
        clearUserPid();
        clearOriginId();
        clearPersonaId();
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
             * returns the logged in user's date of birth
             * @return {string}
             * @method
             */
            dob: getDob
        },

        //These are not exposed by the JSSDK, are used by auth to manage userInfo's internal data.
        setOriginId: setOriginId,
        setAccessToken: setAccessToken,
        setUnderAge: setUnderAge,
        setPersonaId: setPersonaId,
        setUserPid: setUserPid,
        setDob: setDob,

        clearPersonaId: clearPersonaId,
        clearUserPid: clearUserPid,
        clearUserAuthInfo: clearUserAuthInfo,
        clearOriginId: clearOriginId,
        clearAccessToken: clearAccessToken,

    };


});