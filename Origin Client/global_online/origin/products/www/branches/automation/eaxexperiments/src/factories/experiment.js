/**
 * @file experiment.js
 */
(function() {
    'use strict';

    function ExperimentFactory(ExperimentCommFactory) {

        var myEvent = new ExperimentCommFactory.Communicator();

        function init(env, cmsstage, locale, storeFront) {
            return Experiment.init(env, cmsstage, locale, storeFront);
        }

        function processExperimentResponse(response) {
            var cdStr = '';
            if (response.addedCustomDimension) {
                //grab the custom dimension from the sdk
                cdStr = Experiment.getUserCustomDimension();
                myEvent.fire('experiment:updateCustomDimension', cdStr);
            }
            return response.result;
        }

        function inExperiment(inExp, inVariant) {
            return Experiment.inExperiment(inExp, inVariant)
                .then (processExperimentResponse)
                .catch (function() {
                    //probably should log something here...
                    return false;
                });
        }

        function setUser(expUserData) {
            Experiment.setUser(expUserData);

            //fire event that experiment user has been updated
            myEvent.fire('experiment:setUser');
        }

        function clearUser() {
            Experiment.clearUser();

            //fire event that experiment user has been updated
            //right now we don't really need this because we reload the SPA on logout but if we ever change that, this will be needed
            //we may need to re-evaluate this if we need to make network calls to check segments and/or we start supporting anonymous experiments
            myEvent.fire('experiment:setUser');
        }

        return {

            events: myEvent,

            /**
             * initializes the experimentsdk
             * @param {String} locale
             * @param {String} storeFront
             * @return {Promise<Boolean>}
             * @method
             */
            init: init,

            /**
             * passes the user data info into experimentsdk
             * @param {Object} expUserData
             * @method
             */
            setUser: setUser,

            /**
             * clears the user data in the experimentsdk
             * @method
             */
            clearUser: clearUser,

            /**
             * initializes the customDimension var within experimentation sdk with passed in value
             * @param {String} customDimension string representing experiment+variants to which the user belongs
             * @return {Boolean} updated true if passed in value updated with removal of non-active experiments
             * @method
             */
            setUserCustomDimension: Experiment.setUserCustomDimension,

            /**
             * given experiment name and variant (optional), returns whether user is in the experiment or not
             * @param {String} inExp name of the experiment
             * @param {String} inVariant name of the variant to check against (may be optional)
             * @return {Promise<Boolean>}
             * @method
             */
            inExperiment: inExperiment,

            /**
             * clears the custom dimension that is stored in the experiment sdk
             * for now, called at logout so that we don't transfer custom dimension across users
             * @method
             */
            clearCustomDimension: Experiment.clearCustomDimension,

            /**
             * returns the custom dimension that keeps track of the user's active experiments+variants
             * @return {String} customDimension comma delimited list of experiments+variants
             * @method
             */
            getUserCustomDimension: Experiment.getUserCustomDimension,

            /**
             * mixes the two custom dimension strings
             * @method
             * @param {string} cdStr custom dimension string
             * @param (string) cdStr2 second custom dimension string
             * @return {string} cdStrMixed mix of cdStr and cdStr2
             */
            mixAndReplaceCustomDimension: Experiment.mixAndReplaceCustomDimension,
        };
    }

    /**
     * @ngdoc service
     * @name eax-experiments.factories.ExperimentFactory
     * @description
     *
     * Experiment Factory
     */
    angular.module('eax-experiments')
        .factory('ExperimentFactory', ExperimentFactory);
}());
