/*jshint strict: false */
/*jshint unused: false */

define([
    'core/telemetry',
    'usertiming'
], function(telemetry, usertiming) {
    /**
     * Contains functions relating to performance timing
     * @module module:performance
     * @memberof module:Origin
     */

    var performance = window.performance || {},
        performanceEnabled = (performance.mark !== undefined),
        START = 'start',
        END = 'end',
        MEASURE = 'measure';


        function setMarker(markerName) {
            if (performanceEnabled) {
                performance.mark(markerName);
            }
        }

        function setMeasure(measureName, startMarker, endMarker) {
            if (performanceEnabled) {
                performance.measure(measureName, startMarker, endMarker);
            }
        }

        function clearMarker(markerName) {
            performance.clearMarks(markerName);
        }

        function clearMarkersAll() {
            performance.clearMarks();
        }


        function clearMeasure(measureName) {
            performance.clearMeasures(measureName);
        }

        function clearMeasuresAll() {
            performance.clearMeasures();
        }

        function getEntriesByType(type) {
            var entries = [];

            if (performanceEnabled) {
                entries = performance.getEntriesByType(type);
            }
            return entries;
        }

        function getMarker(markerName) {
            var perfMarkers = [];

            if (performanceEnabled) {
                perfMarkers = performance.getEntriesByName(markerName);
            }
            return perfMarkers;
        }

        function getMarkersAll() {
            var perfMarkers = [];
            if (performanceEnabled) {
                perfMarkers = performance.getEntriesByType('mark');
            }
            return perfMarkers;
        }

        function getMeasure(measureName) {
            var perfMeasures = [];

            if (performanceEnabled) {
                perfMeasures = performance.getEntriesByName(measureName);
            }
            return perfMeasures;
        }


        function getMeasuresAll() {
            var perfMarkers = [];
            if (performanceEnabled) {
                perfMarkers = performance.getEntriesByType('measure');
            }
            return perfMarkers;
        }

        //************************************************************
        //*************** HELPER FUNCTIONS ***************************
        //************************************************************
        function beginTime(markerPrefix) {
            //clear existing marker, if there is one
            clearMarker(markerPrefix+START);
            setMarker(markerPrefix+START);
        }

        function endTime(markerPrefix) {
            try {
                //clear existing marker, if there is one
                clearMarker(markerPrefix + END);
                setMarker(markerPrefix + END);
            } catch(e) {
                //prevent errors from leaking into user flow
            }
        }

        function measureTime(markerPrefix) {
            var measure = {},
                measureArray;
            //clear existing measure, if there is one
            clearMeasure(markerPrefix+MEASURE);
            setMeasure(markerPrefix+MEASURE, markerPrefix+START, markerPrefix+END);
            measureArray = getMeasure(markerPrefix+MEASURE);
            if (measureArray.length > 0) {
                measure = measureArray[0];
            }
            return measure;
        }

        function sendMeasureTelemetry(markerPrefix) {
            var markerArray, measureArray,
                startTime = 0,
                endTime = 0,
                duration = 0;

            markerArray = getMarker(markerPrefix+START);
            if (markerArray.length > 0) {
                startTime = markerArray[0].startTime;
            }

            markerArray = getMarker(markerPrefix+END);
            if (markerArray.length > 0) {
                endTime = markerArray[0].startTime;
            }

            measureArray = getMeasure(markerPrefix+MEASURE);
            if (measureArray.length > 0) {
                duration = measureArray[0].duration;
            }
            telemetry.sendPerformanceTimerEvent(markerPrefix, startTime, endTime, duration);

        }

        function endTimeMeasureAndSendTelemetry(markerPrefix) {
            try {
                endTime(markerPrefix);
                measureTime(markerPrefix);
                sendMeasureTelemetry(markerPrefix);
            } catch(e) {
                //prevent errors from leaking into user flow
            }
        }

    return /** @lends Origin.module:performance */{
        /**
         * sets a performance marker
         * @method
         * @static
         * @param {string} markname
         */
        setMarker: setMarker,

        /**
         * given marker start and marker end, sets a measure of specified name
         * @method
         * @static
         * @param {string} measure name
         * @param {string} marker start
         * @param {string} marker end
         */
        setMeasure: setMeasure,

        /**
         * clears specified marker
         * @method
         * @static
         * @param {string} markername
         */
        clearMarker: clearMarker,

        /**
         * clears all markers
         * @method
         * @static
         */
        clearMarkersAll: clearMarkersAll,

        /**
         * clears specified measure
         * @method
         * @static
         * @param {string} measurename measurename
         */
        clearMeasure: clearMeasure,

        /**
         * clears all measures
         * @method
         * @static
         */
        clearMeasuresAll: clearMeasuresAll,

        /**
         * @typedef performanceObject
         * @type object
         * @property {float} duration for mark, 0; for measure, duration between markers supplied
         * @property {string} entryType mark or measure
         * @property {string} name
         * @property {string} startTime
         */

        /**
         * given a marker name, returns the associated performanceObject(s)
         * @method
         * @static
         * @param {string} marker name of marker
         * @return {Origin.module:performance~performanceObject[]} markerObj array of markers
         */
        getMarker: getMarker,

        /**
         * returns all markers
         * @method
         * @static
         * @return {Origin.module:performance~performanceObject[]} markerObj array of markers
         */
        getMarkersAll: getMarkersAll,

        /**
         * given the start marker and end marker, returns the performanceObject(s)
         * @method
         * @static
         * @param {string} measureName
         * @return {Origin.module:performance~performanceObject[]} measureObj array of measures
         */
        getMeasure: getMeasure,

        /**
         * returns all markers
         * @method
         * @static
         * @param {string} measureName
         * @param {string} startMarker
         * @param {string} endMarker
         * @return {Origin.module:performance~performanceObject[]} measureObj array of measures
         */
        getMeasuresAll: getMeasuresAll,

        /**
         * marks the beginning timer, clears existing marker
         * @method
         * @static
         * @param {string} markerPrefix
         */
        beginTime: beginTime,

        /**
         * marks the end of the timer, clears existing marker
         * @method
         * @static
         * @param {string} markerPrefix
         */
        endTime: endTime,

        /**
         * measure the time between start and end marker, clears existing measure
         * @method
         * @static
         * @param {string} markerPrefix
         * @return {Origin.module:performance~performanceObject} measureObj
         */
        measureTime: measureTime,

        /**
         * sends to telemetry info about the specified measure, measureTime needs to have been called prior to this
         * @method
         * @static
         * @param {string} markerPrefix
         */
        sendMeasureTelemetry: sendMeasureTelemetry,

        /**
         * all-in-one function that calls endTime, measureTime and sendMeasureTelemetry
         * @method
         * @static
         * @param {string} markerPrefix
         */
        endTimeMeasureAndSendTelemetry: endTimeMeasureAndSendTelemetry
    };
});