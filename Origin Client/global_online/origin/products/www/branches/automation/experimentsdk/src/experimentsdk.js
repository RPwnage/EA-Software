/*jshint unused: false */
/*jshint strict: false */

define([
    'core/exp',
    'core/expeax',
    'core/user',
    'core/telemetry',
    'core/urls',
], function(exp, expeax, user, telemetry, urls) {

    /**
     * @exports Experiment
     */

    var experimentsdk = {

        /**
         * the version number of the Experiment SDK
         * @method
         * @return {string} name the version number (X.X.X)
         */
        version: function() {
            return '0.0.1';
        },

        /**
         * initialization function for the Experiment SDK
         * @method
         * @param {string} env environment
         * @param {string} cmsstage cmsstage
         * @param {string} locale two 2-letter hyphenated locale, e.g en-us
         * @param {string} storefront 3 letter storefront
         * @return {promise} name resolved indicates the initialization succeed, reject means the intialization failed
         */
        init: function(env, cmsstage, locale, storefront) {

            urls.init(env, cmsstage);

            user.setLocale(locale);
            user.setStoreFront(storefront);

            //once we switch to EADP's system, we won't have to do load segments or experiments (at least for initial implementation)
            //may need to rethink this so caller doesn't stall waiting for this...
            return expeax.loadSegmentsAndExperiments();
        },

        /**
         * initializes the userdata object so that Experiment SDK doesn't have to go retrieve the data on its own
         * @method
         * @param {object} userdata contains user information
         */
        setUser: user.init,

        /**
         * clears the userdata obj
         * @method
         */
        clearUser: user.clearUserObj,

        /**
         * mixes the two custom dimension strings if unique but replaces with variant in newCdStr if exp not unique
         *
         * @method
         * @param {string} cdStr custom dimension string
         * @param (string) newCdStr most recent custom dimension string
         * @return {string} cdStrMixed mix (and replaced) of cdStr and newCdStr
         */
        mixAndReplaceCustomDimension: telemetry.mixAndReplaceCustomDimension,

        /**
         * initializes the custom dimension with passed in value but removes any experiments that are no longer active
         * @method
         * @param {string} cdStr custom dimension string
         * @return {Boolean} updated true if passed in custom dimension was changed due to experiment no longer being active
         */
        setUserCustomDimension: exp.initCustomDimensionForActiveExperiments,

        /**
         * returns true/false for whether the user is in the experiment, and if variant provided, checks if in variant
         * @method
         * @param {string} inExp experiment name
         * @param {string} inVariant variant name
         * @return {Promise<Object>}
         */
        inExperiment: exp.inExperiment,

        /**
         * clears the global customDimension var tht keeps track of all the experiments to which the user belongs
         * @method
         */
        clearCustomDimension: telemetry.clearCustomDimensionList,

        /**
         * retrieves the current value of customDimension
         * @method
         * @return {string} customDimension string
         */
        getUserCustomDimension: telemetry.getCustomDimension,

        /**
         * API to allow programmatic setting of the variantOverride value; will override the data read in from CQ
         * @method
         * @param {String} expName name of epxeriment
         * @param {String} variantName override Variant name
         */
        setVariantOverride: expeax.setVariantOverride,

        /**
         * used to test the distribution of the murmuhash
         * @method
         * @param {string[]} nucleusIds an array of nucleusIds
         * @param {string} uuid unique id
         * @param {...number} percentagelist a list of numbers that represent percentage distribution, e.g. 20, 20, 60
         */
        testDistribution: expeax.testDistribution
    };

    //we want these available even before Origin.init happens
//    experimentsdk.config = experimentsdkconfig;
    return experimentsdk;
});
