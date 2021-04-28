/*jshint strict: false */
/*jshint unused: false */

// Just a pass thru to send a telemetry event

// should have NO dependency on anything that requires Origin to be initialized
define(['core/utils'], function(utils) {
    /**
     * telemtry functions
     * @module module:telemetry
     * @memberof module:Origin
     */

    var myEvents = new utils.Communicator(),

        publicEventEnums = {
            TELEMETRY_EVENT: 'telemetrySendEvent',
            TELEMETRY_PAGEVIEW: 'telemetrySendPageView'
        },

        trackerTypes = {
            TRACKER_MARKETING: 'TRACKER_MARKETING',
            TRACKER_DEV: 'TRACKER_DEV'
        };


    function sendTelemetryEvent() {
        myEvents.fire(publicEventEnums.TELEMETRY_EVENT, arguments);
    }

    function sendLoginEvent(action, type, status, statuscode) {
        /*jshint camelcase:false */
        sendTelemetryEvent(trackerTypes.TRACKER_DEV, 'login', 'user', action, {
           'status': status,
            'type': type,
            'status_code':statuscode
        });
        /*jshint camelcase:true */
    }

    function sendErrorEvent(errMessage, errDescription, toPINdetail) {
        sendTelemetryEvent(trackerTypes.TRACKER_DEV, 'error', errMessage, errDescription, toPINdetail);
    }

    function sendMarketingEvent(category, action, label, params) {
        sendTelemetryEvent(trackerTypes.TRACKER_MARKETING, category, action, label, params);
    }

    function sendPageView(page, params) {
        myEvents.fire(publicEventEnums.TELEMETRY_PAGEVIEW, page, params);
    }

    utils.mix(myEvents, publicEventEnums);


    return /**@ lends Origin.module:telemetry */ {

        events: myEvents,

        /**
         * fires an event to send telemetry
         * passes along arguments in the triggered event
         * @method
         */
        sendTelemetryEvent: sendTelemetryEvent,

        /**
         * fires a login event to dev tracker
         * @method
         * @param {string} action
         * @param {string} type
         * @param {string} status
         * @param {string} statuscode
         */
        sendLoginEvent: sendLoginEvent,

        /**
         * fires an error event to dev tracker
         * @method
         * @param {string} errMessage
         * @param {string} errDescription
         * @param {string} toPINdetail
         */
        sendErrorEvent: sendErrorEvent,

        /**
         * fires an event to the marketing tracker
         * @method
         * @param {string} category event category, e.g. click
         * @param {string} action
         * @param {string} label
         * @param {object} params param object, any additional data
         */
        sendMarketingEvent: sendMarketingEvent,

        /**
         * sends a page view event to marketing tracker
         * @method
         * @param {string} page
         * @param {object} params
         */
        sendPageView: sendPageView
    };
});