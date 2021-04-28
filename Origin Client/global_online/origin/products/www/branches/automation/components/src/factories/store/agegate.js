/**
 * @file store/agegate.js
 */
(function () {
    'use strict';
    var AGE_GATE_COUNTRIES = ['RU', 'GB', 'AU', 'TW', 'DE'],
        AGE_GATE_THRESHOLD = {
            'RU': 18,
            'GB': 12,
            'AU': 15,
            'TW': 18,
            'DE': 18
        },
        RATING_AGE_MAPPING = {
        'BBFC': {
            'BBFC_U': 0,
            'BBFC_PG': 0,
            'BBFC_12': 12,
            'BBFC_12A': 12,
            'BBFC_15': 15,
            'BBFC_18': 18,
            'BBFC_R_18': 18,
            'MATURE': 18
        },
        'CERO': {
            'CERO_A': 0,
            'CERO_B': 12,
            'CERO_C': 15,
            'CERO_D': 17,
            'CERO_Z': 18,
            'MATURE': 18
        },
        'COB': {
            'COB_G': 0,
            'COB_PG': 8,
            'COB_M': 12,
            'COB_MA15PLUS': 15,
            'COB_MA18PLUS': 18,
            'MATURE': 18
        },
        'DJCTQ': {
            'DJCTQ_Livre': 0,
            'DJCTQ_10': 10,
            'DJCTQ_12': 12,
            'DJCTQ_14': 14,
            'DJCTQ_16': 16,
            'DJCTQ_18': 18,
            'MATURE': 18
        },
        'ESRB': {
            'ESRB_RP': 0,
            'ESRB_RP_FRCA': 0,
            'ESRB_RP_ESMX': 0,

            'ESRB_EC': 0,
            'ESRB_EC_FRCA': 0,
            'ESRB_EC_ESMX': 0,

            'ESRB_E': 0,
            'ESRB_E_FRCA': 0,
            'ESRB_E_ESMX': 0,

            'ESRB_E10PLUS': 10,
            'ESRB_E10PLUS_FRCA': 10,
            'ESRB_E10PLUS_ESMX': 10,
            'ESRB_E10_FRCA': 10,

            'ESRB_T': 13,
            'ESRB_T_FRCA': 13,
            'ESRB_T_ESMX': 13,

            'ESRB_M': 17,
            'ESRB_M_FRCA': 17,
            'ESRB_M_ESMX': 17,

            'ESRB_AO': 18,
            'ESRB_AO_FRCA': 18,
            'ESRB_AO_ESMX': 18,

            'MATURE': 18
        },
        'FPB': {
            'FPB_TBC': 0,
            'FPB_A': 0,
            'FPB_PG': 0,
            'FPB_PGV': 0,
            'FPB_10': 10,
            'FPB_10M': 10,
            'FPB_13': 13,
            'FPB_13V': 13,
            'FPB_13VL': 13,
            'FPB_16': 16,
            'FPB_16V': 16,
            'FPB_16VL': 16,
            'FPB_18': 18,
            'FPB_18V': 18,
            'FPB_18VL': 18,

            'MATURE': 18
        },
        'GRB': {
            'GRB_ALL': 0,
            'GRB_12': 12,
            'GRB_15': 15,
            'GRB_18': 18,

            'MATURE': 18
        },
        'MDA': {
            'MDA_G': 0,
            'MDA_PG': 0,
            'MDA_PG13': 13,
            'MDA_NC16': 16,
            'MDA_M18': 18,
            'MDA_R21': 21,

            'MATURE': 18
        },
        'OFLC': {
            'OFLC_CTC': 0,
            'OFLC_G': 0,
            'OFLC_PG': 15,
            'OFLC_M': 18,
            'OFLC_R13': 13,
            'OFLC_R16': 16,
            'OFLC_R18': 18,
            'MATURE': 18
        },
        'PEGI': {
            'PEGI_3': 3,
            'PEGI_4': 4,
            'PEGI_6': 6,
            'PEGI_7': 7,
            'PEGI_12': 12,
            'PEGI_16': 16,
            'PEGI_18': 18,
            'PEGI_3RP_DE': 3,
            'PEGI_3RP_DK': 3,
            'PEGI_3RP_EN': 3,
            'PEGI_3RP_ES': 3,
            'PEGI_3RP_FI': 3,
            'PEGI_3RP_FR': 3,
            'PEGI_3RP_GR': 3,
            'PEGI_3RP_IT': 3,
            'PEGI_3RP_NL': 3,
            'PEGI_3RP_NO': 3,
            'PEGI_3RP_PO': 3,
            'PEGI_3RP_PT': 3,
            'PEGI_3RP_RU': 3,
            'PEGI_3RP_SE': 3,
            'PEGI_3RP_TR': 3,
            'PEGI_4RP_DE': 4,
            'PEGI_4RP_DK': 4,
            'PEGI_4RP_EN': 4,
            'PEGI_4RP_ES': 4,
            'PEGI_4RP_FI': 4,
            'PEGI_4RP_FR': 4,
            'PEGI_4RP_GR': 4,
            'PEGI_4RP_IT': 4,
            'PEGI_4RP_NL': 4,
            'PEGI_4RP_NO': 4,
            'PEGI_4RP_PO': 4,
            'PEGI_4RP_PT': 4,
            'PEGI_4RP_RU': 4,
            'PEGI_4RP_SE': 4,
            'PEGI_4RP_TR': 4,
            'PEGI_6RP_DE': 6,
            'PEGI_6RP_DK': 6,
            'PEGI_6RP_EN': 6,
            'PEGI_6RP_ES': 6,
            'PEGI_6RP_FI': 6,
            'PEGI_6RP_FR': 6,
            'PEGI_6RP_GR': 6,
            'PEGI_6RP_IT': 6,
            'PEGI_6RP_NL': 6,
            'PEGI_6RP_NO': 6,
            'PEGI_6RP_PO': 6,
            'PEGI_6RP_PT': 6,
            'PEGI_6RP_RU': 6,
            'PEGI_6RP_SE': 6,
            'PEGI_6RP_TR': 6,
            'PEGI_7RP_DE': 7,
            'PEGI_7RP_DK': 7,
            'PEGI_7RP_EN': 7,
            'PEGI_7RP_ES': 7,
            'PEGI_7RP_FI': 7,
            'PEGI_7RP_FR': 7,
            'PEGI_7RP_GR': 7,
            'PEGI_7RP_IT': 7,
            'PEGI_7RP_NL': 7,
            'PEGI_7RP_NO': 7,
            'PEGI_7RP_PO': 7,
            'PEGI_7RP_PT': 7,
            'PEGI_7RP_RU': 7,
            'PEGI_7RP_SE': 7,
            'PEGI_7RP_TR': 7,
            'PEGI_12RP_DE': 12,
            'PEGI_12RP_DK': 12,
            'PEGI_12RP_EN': 12,
            'PEGI_12RP_ES': 12,
            'PEGI_12RP_FI': 12,
            'PEGI_12RP_FR': 12,
            'PEGI_12RP_GR': 12,
            'PEGI_12RP_IT': 12,
            'PEGI_12RP_NL': 12,
            'PEGI_12RP_NO': 12,
            'PEGI_12RP_PO': 12,
            'PEGI_12RP_PT': 12,
            'PEGI_12RP_RU': 12,
            'PEGI_12RP_SE': 12,
            'PEGI_12RP_TR': 12,
            'PEGI_16RP_DE': 16,
            'PEGI_16RP_DK': 16,
            'PEGI_16RP_EN': 16,
            'PEGI_16RP_ES': 16,
            'PEGI_16RP_FI': 16,
            'PEGI_16RP_FR': 16,
            'PEGI_16RP_GR': 16,
            'PEGI_16RP_IT': 16,
            'PEGI_16RP_NL': 16,
            'PEGI_16RP_NO': 16,
            'PEGI_16RP_PO': 16,
            'PEGI_16RP_PT': 16,
            'PEGI_16RP_RU': 16,
            'PEGI_16RP_SE': 16,
            'PEGI_16RP_TR': 16,
            'PEGI_18RP_DE': 18,
            'PEGI_18RP_DK': 18,
            'PEGI_18RP_EN': 18,
            'PEGI_18RP_ES': 18,
            'PEGI_18RP_FI': 18,
            'PEGI_18RP_FR': 18,
            'PEGI_18RP_IT': 18,
            'PEGI_18RP_NL': 18,
            'PEGI_18RP_NO': 18,
            'PEGI_18RP_PO': 18,
            'PEGI_18RP_PT': 18,
            'PEGI_18RP_RU': 18,
            'PEGI_18RP_SE': 18,
            'PEGI_18RP_TR': 18,
            'MATURE': 18
        },
        'RGRS': {
            'RGRS_0W': 0,
            'RGRS_6W': 6,
            'RGRS_12W': 12,
            'RGRS_16W': 16,
            'RGRS_18W': 18,
            'MATURE': 18
        },
        'TWAR': {
            'TWAR_0': 0,
            'TWAR_6': 6,
            'TWAR_12': 12,
            'TWAR_15': 15,
            'TWAR_18': 18,
            'MATURE': 18
        },
        'USK': {
            'USK_0': 0,
            'USK_6': 6,
            'USK_12': 12,
            'USK_16': 16,
            'USK_18': 18,
            'MATURE': 18
        }
    },
    MEDIA_AGEGATE_LOCALES = {
        'US': 18,
        'TW': 18
    };

    function StoreAgeGateFactory(LocaleFactory, moment) {
        var currentRatingSystem = LocaleFactory.getRatingSystem(Origin.locale.countryCode());

        /**
         * Extract rating ID from rating system icon
         * This is hacky but untill we can convince EADP to return the actual property, this is the only way
         *
         * Given a rating system icon path, this will product the rating category
         *
         * http://../ESRB_M.png => ESRB_M
         *
         * @param {Object} game catalog information
         * @returns {String} rating category
         * @private
         */
        function extractRatingIdFromSystemIconPath(game){
            var gameRatingSystemIcon = game.gameRatingSystemIcon || game.ratingSystemIcon || '',
                ratingIconName = gameRatingSystemIcon.split('/').pop(),
                ratingIconNameSplitted = ratingIconName.split('.', 1);
            return ratingIconNameSplitted.length > 0 ? ratingIconNameSplitted[0] : '';
        }

        /**
         * Check locale against list of legally required countries
         *
         * @returns {boolean}
         * @private
         */
        function isAgeGateRequired(){
            return (AGE_GATE_COUNTRIES.indexOf(Origin.locale.countryCode()) > -1);
        }

        /**
         * get age requirement from game
         *
         * @param {Object} game catalog information
         * @returns {Integer} age requirement
         *
         * @private
         */
        function getAgeRequirement(game){
            var ratingCategory = extractRatingIdFromSystemIconPath(game).toUpperCase(),
                ageRequirement = RATING_AGE_MAPPING[currentRatingSystem][ratingCategory];
            if (!ageRequirement && game.gameRatingPendingMature){
                ageRequirement = RATING_AGE_MAPPING[currentRatingSystem].MATURE;
            }
            return ageRequirement;
        }

        /**
         * Determine if user is under age given game catalog information
         *
         * @param game
         * @returns {boolean} true if user is under age
         */
        function isUserUnderGameAgeRequirement(game){
            if (!isAgeGateRequired()){
                return false;
            }
            var userDob = Origin.user.dob(),
                ageRequirement = getAgeRequirement(game),
                userAge = moment().diff(userDob, 'years'),
                threshold = AGE_GATE_THRESHOLD[Origin.locale.countryCode()] || 0; //0 for fail safe
            return !userAge || (ageRequirement >= threshold && userAge < ageRequirement);
        }

        /**
         * Validate age requirement and display error dialog
         * Allow chaining of promise
         *
         * @param {Object} game catalog information
         *
         * @returns {Promise} resolve if user is not under age, reject otherwise
         */
        function validateUserAgePromise(game){
            if (game && isUserUnderGameAgeRequirement(game)){
                return Promise.reject('user-under-age');
            }else{
                return Promise.resolve(game);
            }
        }

        function getAge(birthdayString) {
            var age;
            if (birthdayString){
                age = moment().diff(birthdayString, 'years');
            }
            return age;
        }

        function isUserUnderAgeForMediaLocale(birthday){
            var birthdayString = birthday || Origin.user.dob(),
                age = getAge(birthdayString),
                restrictedAge = getMediaLocalAgeGateAge();
            return !_.isNumber(age) || age < restrictedAge;
        }

        function getMediaLocalAgeGateAge(){
            return MEDIA_AGEGATE_LOCALES[Origin.locale.countryCode()];
        }


        return {
            isUserUnderGameAgeRequirement: isUserUnderGameAgeRequirement,
            isUserUnderAgeForMediaLocale: isUserUnderAgeForMediaLocale,
            getMediaLocalAgeGateAge: getMediaLocalAgeGateAge,
            isAgeGateRequired: isAgeGateRequired,
            validateUserAgePromise: validateUserAgePromise
        };
    }

    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup}
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function StoreAgeGateFactorySingleton(LocaleFactory, moment, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('StoreAgeGateFactory', StoreAgeGateFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.StoreAgeGateFactory

     * @description
     *
     * Age gate
     */
    angular.module('origin-components')
        .factory('StoreAgeGateFactory', StoreAgeGateFactorySingleton);
})();
