/*jshint unused: false */
/*jshint strict: false */
define([
    'core/logger',
], function(logger) {
    var VALUE_CLEARED = '';

    var data = {
        auth: {
            accessToken: ''
        },
        originId: '',
        personaId: '',
        userPID: '',
        underAge: false,
        subscription: false,
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

    function setSubscription(flag) {
        data.subscription = flag;
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
        //MY: this is simply to allow QA to verify that DOB is correctly being set even in offline mode
        //once QA verifies, we can remove this logging
        logger.log('USER setDOB:', dob);
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

    function getSubscription() {
        return data.subscription;
    }

    return {

        //These are exposed by the JSSDK
        publicObjs: {
            /**
             * @method
             */
            accessToken: getAccessToken,

            /**
             * return logged in user's originId
             * @method
             */
            originId: getOriginId,

            /**
             * return logged in user's personaId
             * @method
             */
            personaId: getPersonaId,

            /**
             * @method
             */
            userPid: getUserPid,

            /**
             * @method
             */
            underAge: getUnderage,

            /**
             * @method
             */
            dob: getDob,

            /**
             * @method
             */
            subscription: getSubscription

        },

        //These are not exposed by the JSSDK, are used by auth to manage userInfo's internal data.
        setOriginId: setOriginId,
        setAccessToken: setAccessToken,
        setUnderAge: setUnderAge,
        setSubscription: setSubscription,
        setPersonaId: setPersonaId,
        setUserPid: setUserPid,
        setDob: setDob,

        clearPersonaId: clearPersonaId,
        clearUserPid: clearUserPid,
        clearUserAuthInfo: clearUserAuthInfo,
        clearOriginId: clearOriginId,
        clearAccessToken: clearAccessToken

    };


});