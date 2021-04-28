/*jshint strict: false */
/*jshint unused: false */
/*jshint latedef: nofunc */
define([
], function() {
    /**
     * Contains logic associated with keeping track of the metric label for experiments
     * @module module:telemtry
     * @memberof module:Experiment
     * @private
     */

    var customDimensionList = [], //internally keep the custom dimensions as a split list, but externally, always return them as concatenated string
        customDimensionStr = '';

    function setCustomDimensionStr() {
        customDimensionStr = customDimensionList.join(',');
    }

    function initCustomDimensionList(cdStrList) {
        customDimensionList = cdStrList;
        setCustomDimensionStr();
    }

    function clearCustomDimensionList() {
        customDimensionList = [];
        setCustomDimensionStr();
    }

    /**
     * checks to see if the expName matches expName in string = expName|expVar
     */
    function matchesExperiment(expName, expNameVar) {
        var expVarList = expNameVar.split('|');
        return (_.get(expVarList, '0') === expName);
    }

    /**
     * checks the list to see if expName already exists in cdlist
     * cdlist is a list of strings <expName|<variant>
     */
    function indexOfExpName(expName, cdList) {
        var expVarList;

        for (var i = 0; i < cdList.length; i++ ) {
            if (matchesExperiment(expName, cdList[i])) {
                return i;
            }
        }
        return -1;
    }

    function mixAndReplaceCustomDimension(cdStr, newCdStr) {
        var combinedCDstr = cdStr,
            cdStrList = cdStr ? cdStr.split(',') : [],
            newCdStrList,
            expNameVar,
            expName,
            expNdx,
            combinedList;

        if (newCdStr !== '') {
            newCdStrList = newCdStr.split(',');

            //newCdStr takes precedence so iterate over that and remove
            //any in CdStr that shares the same experiment
            for (var i = 0; i < newCdStrList.length; i++) {
                expNameVar = newCdStrList[i].split('|');
                if (expNameVar.length > 0) {
                    expName = expNameVar[0];

                    //handle the case when we inadvertently added multiple entries for the experiment due to UUID not being consistent when promoting across
                    //cms stages; so iterate over and remove all existing
                    _.remove(cdStrList, _.partial(matchesExperiment, expName));
                }
            }

            //now combine newCdStrList with what's left over in cdStrList
            combinedList = newCdStrList.concat(cdStrList);
            combinedCDstr = combinedList.join(',');
        }
        return combinedCDstr;
    }


    function updateCustomDimension (expName, variant) {
        var cdStr,
            add = false,
            expNdx = -1,
            expVarList,
            expVar,
            remove = false,
            removedCdStr = '';

        //check and see if expName exists in custom dimension already
        expNdx = indexOfExpName(expName, customDimensionList);

        if (expNdx < 0) {
            //doesn't already exist, so we'll just add it
            add = true;
        } else {
            //assume that we keep the variants per experiment mutually exclusive (i.e. we don't have exp1|variantA AND exp1|variantB stored as custom dimension)
            expVar = customDimensionList[expNdx].split('|');
            //5 scenarios
            //1) existing has no variant (just experiment)
            //  a) but now we want to add variant so => remove existing = true, add = true
            //  b) new setting also has no variant so no need to update => remove existing = false, add = false
            //2) existing has variant
            //  a) new setting has no variant so => remove existing = true, add = true
            //  b) new setting has variant, doesn't match existing variant so => remove existing = true, add = true
            //  c) new setting variant matches existing variant so => remove existing = false, add = false
            if (expVar.length === 1 && variant.length > 0) { //1a
                remove = true;
                add = true;
            } else if (expVar.length > 1 && expVar[1] !== variant) { //2a & 2b
                remove = true;
                add = true;
            }

            if (remove) {
                customDimensionList.splice(expNdx, 1);
            }
        }

        if (add) {
            cdStr = expName;
            if (variant) {
                cdStr += '|' + variant;
            }

            customDimensionList.push(cdStr);
            setCustomDimensionStr();
        }
        return add;
    }

    return /** @lends module:Experiment.module:telemetry */ {

        /**
         * initializes custom dimension with parameter passed in
         * @param {string} cdStr custom dimension string
         * @method
         */
        initCustomDimensionList: initCustomDimensionList,

        /**
         * clears the global customDimension var that keeps track of all the experiments to which the logged in user belongs
         * @method
         */
        clearCustomDimensionList: clearCustomDimensionList,

        /**
         * mixes the two custom dimension strings if unique but replaces with variant in newCdStr if exp not unique
         * this is a utility function and does not update the vars in this module
         *
         * @method
         * @param {string} cdStr custom dimension string
         * @param (string) newCdStr most recent custom dimension string
         * @return {string} cdStrMixed mix (and replaced) of cdStr and newCdStr
         */
        mixAndReplaceCustomDimension: mixAndReplaceCustomDimension,

        /**
         * retrieves the current value of customDimension
         * @method
         * @return {string} customDimension string
         */
        getCustomDimension: function () {
            return customDimensionStr;
        },

        /**
         * sets/add the experiment and variant to the custom dimension, if a previous variant exists for same experiment, remove that first before setting new variant
         * @method
         * @param {string} expName name of the experiment
         * @param {variant} variant to which the user belongs
         * @return {Boolean} add true/false on whether it was added to custom dimension
         */
        updateCustomDimension: updateCustomDimension
    };

});