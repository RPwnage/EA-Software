/*jshint strict: false */
/*jshint unused: false */
/*jshint latedef: nofunc */
define([
    'core/user',
    'core/urls',
    'core/xhr',
    'core/telemetry',
    'core/murmurhash',
], function(user, urls, xhr, telemetry, murmurhash) {
    /**
     * Contains logic associated with running experiments
     * @module module:exp
     * @memberof module:Experiment
     * @private
     */

    var segmentsLoaded = false,
        experimentsLoaded = false;

    var VALID_ATTRIBUTES = [
            'locales',
            'storeFronts',
            'originAccessUser'
        ],
        TEST_FUNCTIONS = {
            'locales' : testInLocales,
            'storeFronts' : testInStoreFronts,
            'originAccessUser' : testOriginAccessUser
        },
        experiments = {},
        segments = {};

    function validateAttribute (key) {
        return (VALID_ATTRIBUTES.indexOf(key) > -1);
    }

    function setTestFn (key) {
        var fn = TEST_FUNCTIONS.hasOwnProperty(key) ? TEST_FUNCTIONS[key] : null;
        return fn;
    }

    function testInLocales(localeArray) {
        var userObj = user.getUserObj();
        return (userObj && localeArray && localeArray.indexOf(userObj.locale) !== -1);
    }

    function testInStoreFronts(storeFrontArray) {
        var userObj = user.getUserObj();
        return (userObj && storeFrontArray && storeFrontArray.indexOf(userObj.storeFront) !== -1);
    }

    function testOriginAccessUser(checkUser) {
        var userObj = user.getUserObj();
        return (userObj.subscriber === checkUser);
    }

    function testEntitlement (attrib) {
        var hasEntitlement = false,
            userObj = user.getUserObj ();

        if (userObj.entitlements && userObj.entitlements.indexOf(attrib) > -1) {
            hasEntitlement = true;
        }
        return hasEntitlement;
    }

    function parseExperimentData(expData) {
        var exp;

        experiments = expData;

        //go thru each one and convert the start and end date to Date()
        for (var key in experiments) {
            exp = experiments[key];
            exp.startDate = new Date(_.get(exp, 'startDate'));

            //if it's empty then leave it empty, means experiment will run until it is removed
            exp.endDate = _.isEmpty(_.get(exp, 'endDate')) ? _.get(exp, 'endDate') : new Date(_.get(exp, 'endDate'));
        }
        experimentsLoaded = true;
        return experimentsLoaded;
    }

    function loadExperiments() {
        var url = urls.endPoints.experiments,
            config = {
                atype: 'GET',
                headers: [],
                parameters: [],
                appendparams: [],
                reqauth: false,
                requser: false
            };

        return xhr.httpRequest(url, config)
            .then(parseExperimentData)
            .catch(function(error) {
                console.log('loadExperiments failed:', error);
                return Promise.reject(error);
            });
    }

    function parseSegmentData(segmentData) {
        segments = segmentData;

        for (var key in segments) {
            for (var label in segments[key].rule.parameters) {
                if (validateAttribute(label) && segments[key].rule.parameters[label]) {
                    segments[key].rule.parameters[label].testFn = TEST_FUNCTIONS[label];
                }
            }
        }
        segmentsLoaded = true;
        return segmentsLoaded;
    }

    function loadSegments() {
        var url = urls.endPoints.segments,
            config = {
                atype: 'GET',
                headers: [],
                parameters: [],
                appendparams: [],
                reqauth: false,
                requser: false
            };

        return xhr.httpRequest(url, config)
            .then(parseSegmentData)
            .catch(function(error) {
                console.log('loadExperiments failed:', error);
                return Promise.reject(error);
            });
    }


    function loadSegmentsAndExperiments() {
        return loadSegments()
            .then(loadExperiments);
    }

    //from Optimizely JS SDK, Apache License 2.0
    function generateBucketValue (bucketingId) {
        var HASH_SEED = 1,
            MAX_HASH_VALUE = Math.pow(2, 32),
            MAX_TRAFFIC_VALUE = 10000,

            hashValue = murmurhash.v3(bucketingId, HASH_SEED),
            ratio = hashValue / MAX_HASH_VALUE;

        return parseInt(ratio * MAX_TRAFFIC_VALUE, 10);
    }

    function getVariantName(expName, bucketValue) {
        var accumulatedPercentage = 0,
            variantsArray = experiments[expName].variants,
            variantName = '';

        if (variantsArray) {
            for (var i = 0; i < variantsArray.length; i++) {
                accumulatedPercentage += (variantsArray[i].percentage * 100);
                if (bucketValue < accumulatedPercentage) {
                    variantName = variantsArray[i].name;
                    break;
                }
            }
        }
        return variantName;
    }

    function checkSegments(expName) {
        var inSegment = false,
            expSegments,
            numSegments = 0,
            segmentName = '',
            segmentObj,
            ruleObj,
            logicResult,
            result;

        //if experiment doesn't exist or segment obj is missing, just assume false
        if (!experiments[expName] || !experiments[expName].segments) {
            return false;
        }

        numSegments = experiments[expName].segments.length;
        if (numSegments === 0) {
            //no segments specified
            inSegment = true;
        }

        for (var i = 0; i < numSegments; i++) {
            segmentName = experiments[expName].segments[i];

            if (segments[segmentName]) {
                ruleObj = segments[segmentName].rule;

                //set up the default for when failure isn't encountered
                if (ruleObj.operator === 'OR') {
                    inSegment = false;
                } else {
                    inSegment = true;
                }

                for (var label in ruleObj.parameters) {
                    result = ruleObj.parameters[label].testFn.call(this, ruleObj.parameters[label].arg);
                    if (ruleObj.operator === 'OR' && result) {
                        //if it's OR then all we need is the first true
                        inSegment = true;
                        break; //go test the next segment
                    } else if (ruleObj.operator === 'AND' && !result) {
                        inSegment = false;
                        //but if it's AND then abort after first false;
                        break;
                    }
                }

                //failed the segment so no need to test other segements, so break out of the for loop
                if (!inSegment) {
                    break;
                }
            } else {
                //referenced segment is missing, just abort out of the check
                inSegment = false;
                break;
            }
        }

        return inSegment;
    }

    function getBucket(expName, expVariant) {
        var bucketed = false,
            userObj = user.getUserObj (),
            nucleusId = userObj.userPID,
            bucketingId,
            bucketValue,
            variantName,
            addedCustomDimension = false,
            bucketedObj = {
                'result': bucketed,
                'addedCustomDimension': addedCustomDimension
            };

        //for now, we're only dealing with logged in users
        if (nucleusId !== '' && checkSegments(expName)) {
            //now that we're in the segment

            //if variantoverride is defined for non-prod, then always bucket the user to that variant
            if (experiments[expName].variantOverride && experiments[expName].variantOverride !== '') {
                variantName = experiments[expName].variantOverride;
            } else {
                uuid = experiments[expName].experimentId;
                bucketingId = nucleusId+uuid;
                bucketValue = generateBucketValue(bucketingId);

                variantName = getVariantName(expName, bucketValue);
                console.log('bucketValue = ', bucketValue, ' variantName=', variantName);
            }

            bucketed = expVariant ? (variantName === expVariant) : true;

            //since we're in here only if the user is in the experiment, we still want to indicate that and the variant that the user is in.
            //bucketed will be true/false based on if expVariant matches the user's bucket variant but as far as telemetry goes
            //we still want to report it as being in the experiment
            //e.g. if variantName = control, and expVariant - showbutton, then the user wouldn't be bucketed
            //but we want to indicate that this user was in the "control" of the experiment
            addedCustomDimension = telemetry.updateCustomDimension(expName, variantName);

            bucketedObj.result = bucketed;
            bucketedObj.addedCustomDimension = addedCustomDimension;

        }
        return bucketedObj;
    }

    function experimentActive(expName) {
        var inTimeInterval = false;

        if (expName in experiments) {
            if (_.get(experiments, [expName, 'startDate']) <= Date.now()) {
                //there may not be an end date specified
                inTimeInterval = (_.isDate(_.get(experiments, [expName, 'endDate']))) ? _.get(experiments, [expName, 'endDate']) > Date.now() : true;
            }
        }
        return inTimeInterval;
    }

    //once we migrate to EADP's system, we'll just get a response that tells me which segments the user is in
    //so let's mimic the response
    function inExperiment(expName, expVariant) {
        var resultObj = {
                result: false,
                addedCustomDimension: false
            };

        //make sure experiments and segments have been loaded
        if (segmentsLoaded && experimentsLoaded) {
            if (experimentActive(expName)) {
                resultObj = getBucket(expName, expVariant);
            }
        }
        return Promise.resolve(resultObj);
    }

    function testDistribution() {
        var numArgs,
            nucleusId,
            uuid,
            bucketId,
            bucketingId,
            nucIds,
            nucIdLen,
            bucketValue,
            dist = [],
            bucketed = [],
            nucleusIdBucket = {},
            numDist = 0,
            expName = '',
            i, j,
            prevdist,
            distObj = {},
            actualTotal;

        numArgs = arguments.length;

        //early return if we don't have nucIds, uuid and at least one distribution
        if (numArgs < 3) {
            return {};
        }

        //arg = 1 is list of nucIds
        nucIds = arguments[0];
        uuid = arguments[1];

        numDist = numArgs - 2;

        //set up the distribution percentage values
        //e.g. if 20, 20, 60 is passed in
        //dist[0] = 20
        //dist[1] = 40
        //dist [2] = 100
        prevdist = 0;
        for (var a = 2; a < numArgs; a++) {
            dist [a-2] = (arguments[a] + prevdist) * 100;
            prevdist += arguments[a];
        }

        //initialize bucketed which keeps track of bucketed distribution
        for (var d = 0; d < numDist; d++) {
            bucketed [d] = 0;
        }

        nucIdLen = nucIds.length;

        for (i = 0; i < nucIdLen; i++) {
            nucleusId = nucIds [i];

            bucketingId = nucleusId+uuid;
            bucketValue = generateBucketValue(bucketingId);

            for (j = 0; j < numDist; j++) {
                if (bucketValue < dist[j]) {
                    bucketed[j]++;
                    break;
                }
            }
        }

        distObj.expectedTotal = nucIdLen;
        actualTotal = 0;

        distObj.bucketedTotals = [];
        distObj.bucketedPercentages = [];
        for (i = 0; i < numDist; i++) {
            distObj.bucketedTotals [i] = bucketed[i];
            distObj.bucketedPercentages[i] = bucketed[i] / nucIdLen;
            actualTotal += bucketed [i];
        }
        distObj.actualTotal = actualTotal;

        return distObj;
    }

    function setVariantOverride(expName, overrideVariant) {
        //overwrite the cached experiment variantOverride
        if (experiments[expName]) {
            experiments[expName].variantOverride = overrideVariant;
        }
    }

    return /** @lends module:Experiment.module:exp */ {

        /**
         * @method
         * @return {Promise}
         */
        loadSegmentsAndExperiments: loadSegmentsAndExperiments,

        /**
         * returns whether experiment is currently active
         * @method
         * @param {string} expName
         * @return {Boolean}
         */
        experimentActive: experimentActive,

        /**
         * given expName and expVariant, returns true/false as to whether user is bucketed in expVariant of expName
         * @method
         * @param {String} expName
         * @param {String} expVariant
         * @return {Promise<Object>}
         */
        inExperiment: inExperiment,

        /**
         * API to allow programmatic setting of the variantOverride value; will override the data read in from CQ
         * @method
         * @param {String} expName name of epxeriment
         * @param {String} variantName override Variant name
         */
        setVariantOverride: setVariantOverride,

        /**
         * @method
         * @param {string[]} nucleusIds an array of nucleusIds
         * @param {string} uuid unique id
         * @param {...number} percentagelist a list of numbers that represent percentage distribution, e.g. 20, 20, 60
          */
        testDistribution: testDistribution
    };

});