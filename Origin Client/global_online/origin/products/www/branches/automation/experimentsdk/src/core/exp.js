/*jshint strict: false */
/*jshint unused: false */
/*jshint latedef: nofunc */
define([
    'core/expeax',
    'core/telemetry'
], function(expeax, telemetry) {
    /**
     * Contains logic associated with running experiments
     * @module module:exp
     * @memberof module:Experiment
     * @private
     */

    //once we migrate to EADP's system, we'll switch to using that module instead of eaxexp
    function inExperiment(expName, expVariant) {
        return expeax.inExperiment(expName, expVariant);
    }

    //once we migrate to EADP's system, we'll switch to using that module instead of eaxexp
    function experimentActive(expName) {
        return expeax.experimentActive(expName);
    }

    function initCustomDimensionForActiveExperiments(cdStr) {
        var cdExperimentList = cdStr.split(','),
            activeCdExperimentList = [],
            expPlusVariant = [],
            activeCdStr;

        for (var i = 0; i < cdExperimentList.length; i++) {
            expPlusVariant = cdExperimentList[i].split ('|');  //extract out the experiment name

            if (expPlusVariant.length > 0 && experimentActive(expPlusVariant[0])) {
                activeCdExperimentList.push(cdExperimentList[i]);
            }
        }

        telemetry.initCustomDimensionList(activeCdExperimentList);
        activeCdStr = activeCdExperimentList.join(',');
        return (activeCdStr !== cdStr);
    }

    return /** @lends module:Experiment.module:exp */ {
        /**
         * given expName and expVariant, returns true/false as to whether user is bucketed in expVariant of expName
         * @method
         * @param {String} expName
         * @param {String} expVariant
         * @return {Promise<Object>}
         */
        inExperiment: inExperiment,

        /**
         * initializes the custom dimension with passed in value afte it removes any experiments that are no longer active
         * @method
         * @param {string} cdStr custom dimension string
         * @return {Boolean} updated true if passed in value updated with removal of non-active experiments
         */
        initCustomDimensionForActiveExperiments: initCustomDimensionForActiveExperiments
    };
});