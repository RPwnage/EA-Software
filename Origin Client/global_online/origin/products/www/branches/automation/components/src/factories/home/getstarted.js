/**
 * @file home/getstarted.js
 */
/* jshint latedef:nofunc */
(function() {
    'use strict';

    function GetStartedFactory(UtilFactory, SettingsFactory, ComponentsLogFactory, DateHelperFactory) {

        var gettingStartedSettings = {
            getStartedCustomAvatar: true,
            getStartedAchievementVisibility: true,
            getStartedProfileVisibility: true,
            getStartedOriginEmail: true,
            getStartedRealName: true,
            getStartedVerifyEmail: true,
            getStartedTwofactorAuth: true
        },
        myEvents = new Origin.utils.Communicator(),
        settingsRetrieved = false,
        remindMeLater,
        userInfo,
        userNameSet = false,
        usrAvatarSet = false;

        function checkDates(date) {
            var originalDate = new Date(parseInt(date));
            if (DateHelperFactory.isValidDate(originalDate)) {
                var futureDate = DateHelperFactory.addDays(originalDate, parseInt(remindMeLater));
                if (DateHelperFactory.isInThePast(futureDate)) {
                    return true;
                }
            }
            return false;
        }

        function setRemindMeLater(date) {
            remindMeLater = date;
        }

        function checkSeenSettings(settingsData) {

             // if the setting is undefined assume it's never been set and return true
            gettingStartedSettings.getStartedCustomAvatar = (settingsData.getStartedCustomAvatar ? checkDates(settingsData.getStartedCustomAvatar) : true) && !usrAvatarSet;
            gettingStartedSettings.getStartedAchievementVisibility = settingsData.getStartedAchievementVisibility ? checkDates(settingsData.getStartedAchievementVisibility) : true;
            gettingStartedSettings.getStartedProfileVisibility = settingsData.getStartedProfileVisibility ? checkDates(settingsData.getStartedProfileVisibility) : true;
            gettingStartedSettings.getStartedOriginEmail = (settingsData.getStartedOriginEmail ? checkDates(settingsData.getStartedOriginEmail) : true ) && !Origin.user.globalEmailSignup();
            gettingStartedSettings.getStartedRealName = (settingsData.getStartedRealName ? checkDates(settingsData.getStartedRealName) : true) && !userNameSet ;
            gettingStartedSettings.getStartedVerifyEmail = (settingsData.getStartedVerifyEmail ? checkDates(settingsData.getStartedVerifyEmail) : true ) && Origin.user.emailStatus() !== 'VERIFIED';
            gettingStartedSettings.getStartedTwofactorAuth = (settingsData.getStartedTwofactorAuth ? checkDates(settingsData.getStartedTwofactorAuth) : true) && !Origin.user.tfaSignup();

            settingsRetrieved = true;
            return settingsRetrieved;
        }

        function retrieveSettingError(error) {
            ComponentsLogFactory.error('[origin-home-newuser-tile]:retrieveSettings FAILED:', error);
        }

        function retrieveUserInfoError(error) {
            ComponentsLogFactory.error('OriginProfileHeaderCtrl: Origin.atom.atomUserInfoByUserIds failed', error);
        }

        function settingsIntialized() {
            return settingsRetrieved;
        }

        function sendEmailVerification() {
            Origin.settings.email.sendEmailVerification();
        }

        function sendOptinEmailSetting() {
            Origin.settings.email.sendOptinEmailSetting();
        }

        function dismiss(name, date) {
            // update our local variable
            gettingStartedSettings[name] = false;
            if (date) {
                // tell the server
                SettingsFactory.setGeneralSetting(name, date);
            }
            // tell anyone who cares
            myEvents.fire("getstarted:settingChanged", name);
        }

        function getSetting(name) {
            return !!gettingStartedSettings[name];
        }

        function checkSetting(name) {
            if (settingsRetrieved) {
                return getSetting(name);
            } else {
                return getData().then(_.partial(getSetting, name));
            }
        }

        function clearAllSettings() {
            for (var setting in gettingStartedSettings) {
                SettingsFactory.setGeneralSetting(setting.name, '');
            }
        }

        function retrieveUserName() {
            return Origin.atom.atomUserInfoByUserIds([Origin.user.userPid()]);
        }

        function processUserName(response) {
            userInfo = response[0];
            if (userInfo) {
                if (userInfo.firstName || userInfo.lastName) {
                    userNameSet = true;
                }
            }

        }

        function processAvatar(response) {
            var avatarInfo = response[0];
            if (avatarInfo) {
                // this check should pass for all environements id check will not
                usrAvatarSet = avatarInfo.avatar.galleryName !== 'default';
            }
        }

        /**
         * Randomize tile order and sort any prioritized tiles
         * @param {Array} tiles
         */
        function shuffleAndSortBucket(tiles) {
            return _.chain(tiles)
                    .shuffle()
                    .sortByOrder('priority', 'desc')
                    .value();
        }

        function getData() {
            return Origin.avatar.avatarInfoByUserIds(Origin.user.userPid(), Origin.defines.avatarSizes.LARGE)
            .then(processAvatar)
            .then(retrieveUserName)
            .catch(retrieveUserInfoError)
            .then(processUserName)
            .then(SettingsFactory.getGeneralSettings)
            .then(checkSeenSettings)
            .catch(retrieveSettingError);
        }

        return {
            dismiss: dismiss,
            checkSetting: checkSetting,
            events: myEvents,
            initialized: settingsIntialized,
            sendEmailVerification: sendEmailVerification,
            sendOptinEmailSetting: sendOptinEmailSetting,
            clearAllSettings: clearAllSettings,
            setRemindMeLater: setRemindMeLater,
            getData: getData,
            shuffleAndSortBucket: shuffleAndSortBucket
        };

    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GetStartedFactorySingleton(UtilFactory, SettingsFactory, ComponentsLogFactory, DateHelperFactory, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GetStartedFactory', GetStartedFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GetStartedFactory

     * @description
     *
     * GetStartedFactory
     */
    angular.module('origin-components')
        .factory('GetStartedFactory', GetStartedFactorySingleton);
}());
