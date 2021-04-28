/*jshint unused: false */
/*jshint strict: false */
define([
    'core/user',
    'core/dataManager',
    'core/urls',
    'core/errorhandler',
], function(user, dataManager, urls, errorhandler) {

    /**
     * @module module:achievements
     * @memberof module:Origin
     */

    function userAchievements(personaId, achievementSet, locale) {
        var endPoint = urls.endPoints.userAchievements;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Api-Version',
                'val': '2'
            },{
                'label': 'X-Application-Key',
                'val': 'Origin'
            }],
            parameters: [{
                'label': 'personaId',
                'val': personaId
            },{
                'label': 'achievementSet',
                'val': achievementSet
            },{
                'label': 'locale',
                'val': locale
            }],
            appendparams: [],
            reqauth: true,
            requser: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'X-AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0)
          .catch(errorhandler.logAndCleanup('ACHIEVEMENTS:userAchievements FAILED'));
    }

    function userAchievementSets(personaId, locale) {
        var endPoint = urls.endPoints.userAchievementSets;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Api-Version',
                'val': '2'
            },{
                'label': 'X-Application-Key',
                'val': 'Origin'
            }],
            parameters: [{
                'label': 'personaId',
                'val': personaId
            },{
                'label': 'locale',
                'val': locale
            }],
            appendparams: [],
            reqauth: true,
            requser: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'X-AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0)
          .catch(errorhandler.logAndCleanup('ACHIEVEMENTS:userAchievementSets FAILED'));
    }

    function userAchievementPoints(personaId) {
        var endPoint = urls.endPoints.userAchievementPoints;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Api-Version',
                'val': '2'
            },{
                'label': 'X-Application-Key',
                'val': 'Origin'
            }],
            parameters: [{
                'label': 'personaId',
                'val': personaId
            }],
            appendparams: [],
            reqauth: true,
            requser: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'X-AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0)
          .catch(errorhandler.logAndCleanup('ACHIEVEMENTS:userAchievementPoints FAILED'));
    }

    function achievementSetReleaseInfo(locale) {
        var endPoint = urls.endPoints.achievementSetReleaseInfo;
        var config = {
            atype: 'GET',
            headers: [{
                'label': 'X-Api-Version',
                'val': '2'
            },{
                'label': 'X-Application-Key',
                'val': 'Origin'
            }],
            parameters: [{
                'label': 'locale',
                'val': locale
            }],
            appendparams: [],
            reqauth: true,
            requser: false
        };

        var token = user.publicObjs.accessToken();
        if (token.length > 0) {
            dataManager.addHeader(config, 'X-AuthToken', token);
        }

        return dataManager.enQueue(endPoint, config, 0)
          .catch(errorhandler.logAndCleanup('ACHIEVEMENTS:achievementSetReleaseInfo FAILED'));
    }

    return /** @lends module:Origin.module:achievements */ {

        /**
         * Achievement icon set
         * @typedef achievementIconsObject
         * @type {object}
         * @property {string} 40 40x40 icon URL
         * @property {string} 208 208x208 icon URL
         * @property {string} 416 416x416 icon URL
         */
        /**
         * Achievement expansion info
         * @typedef achievementExpansionObject
         * @type {object}
         * @property {string} id
         * @property {string} name
         */
        /**
         * Achievement info
         * @typedef achievementObject
         * @type {object}
         * @property {string} achievedPercentage percentage as a string to two decimal places, i.e. 0.00
         * @property {number} cnt the number of times this achievement has been earned
         * @property {string} desc localized description of this achievement
         * @property {number} e timestamp of expiration date for this achievement, in msec since epoch
         * @property {achievementIconsObject} icons icons of various sizes
         * @property {string} howto localized description of how to earn this achievement
         * @property {string} img unlocalized name of the image associated with this achievement
         * @property {string} name localized achievement name
         * @property {number} p progress points towards this achievement
         * @property {number} pt TBD
         * @property {object[]} requirements TBD
         * @property {number} rp reward points
         * @property {string} rp_t reward points (visualized)
         * @property {number} s TBD
         * @property {boolean} supportable TBD
         * @property {number} t total progress points needed to earn this achievement
         * @property {number} tc TBD
         * @property {boolean} tiered TBD
         * @property {object[]} tn TBD
         * @property {number} tt TBD
         * @property {number} u timestamp of last update to this achievement, in msec since epoch
         * @property {number} xp experience points
         * @property {string} xp_t experience points (visualized)
         * @property {achievementExpansionObject} xpack if this achievement is associated with an expansion, this field will contain information about the expansion
         */
        /**
         * Achievement set info
         * @typedef achievementSetObject
         * @type {object}
         * @property {achievementObject[]} achievements indexed by achievement ID
         * @property {achievementExpansionObject[]} expansions
         * @property {string} name
         * @property {string} platform
         */
        /**
         * Achievement progress info
         * @typedef achievementProgressObject
         * @type {object}
         * @property {number} achievements
         * @property {number} rewardPoints
         * @property {number} experience
         */
        /**
         * Published achievement product info
         * @typedef publishedAchievementProductObject
         * @type {object}
         * @property {string} masterTitleId
         * @property {string} name
         * @property {string} platform
         */

        /**
         * Retrieve all achievements for a user
         * @param  {personaId} personaId of the user
         * @param  {achievementSet} achievementSetId of the achievement set
         * @param  {locale} current application locale
         * @return {Promise<achievementSetObject>} A promise that on success will return all achievements from a specific achievement set for a user.
         */
        userAchievements: userAchievements,

        /**
         * Retrieve all achievement sets for a user
         * @param  {personaId} personaId of the user
         * @param  {locale} current application locale
         * @return {Promise<achievementSetObject[]>} A promise that on success will return all applicable achievement sets indexed by achievementSetId for a user.
         */
        userAchievementSets: userAchievementSets,

        /**
         * Retrieve achievement points for a user
         * @param  {personaId} personaId of the user
         * @return {Promise<achievementProgressObject>} A promise that on success will return the Achievement Progression for a user.
         */
        userAchievementPoints: userAchievementPoints,

        /**
         * Retrieves all released achievement sets
         * @param  {locale} current application locale
         * @return {Promise<publishedAchievementProductObject[]>} A promise that on success with a list of offers indexed by achievementSetId that have achievements and
         * are published both in the catalog and the achievement service.
         */
        achievementSetReleaseInfo: achievementSetReleaseInfo

    };
});
