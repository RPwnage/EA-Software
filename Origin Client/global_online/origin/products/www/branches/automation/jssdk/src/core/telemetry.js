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
            TELEMETRY_PIN_EVENT: 'telemetryPinSendEvent',
            TELEMETRY_PAGEVIEW: 'telemetrySendPageView',
            TELEMETRY_TRANSACTION_EVENT: 'telemetryTransactionEvent',
            TELEMETRY_CLIENT_ACTION: 'telemetryClientAction',
            TELEMETRY_PERFORMANCE_TIMER: 'telemetryPerformanceTimer',
            TELEMETRY_CUSTOM_DIMENSION: 'telemetrySetCustomDimension',

            // social events
            TELEMETRY_FRIEND_REMOVE: 'telemetryFriendRemove',
            TELEMETRY_FRIEND_REMOVE_BLOCK: 'telemetryFriendRemoveBlock',
            TELEMETRY_FRIEND_BLOCK: 'telemetryFriendBlock',
            TELEMETRY_FRIEND_UNBLOCK: 'telemetryFriendUnblock',
            TELEMETRY_FRIEND_REQUEST_ACCEPT: 'telemetryFriendRequestAccept',
            TELEMETRY_FRIEND_REQUEST_REJECT: 'telemetryFriendRequestReject',
            TELEMETRY_FRIEND_REQUEST_SEND: 'telemetryFriendRequestSend',
            TELEMETRY_FRIEND_REQUEST_CANCEL: 'telemetryFriendRequestCancel',
            TELEMETRY_FRIEND_REQUEST_CANCEL_BLOCK: 'telemetryFriendRequestCancelBlock',
            TELEMETRY_FRIEND_REQUEST_IGNORE_BLOCK: 'telemetryFriendRequestIgnoreBlock',
            // friend recommendation
            TELEMETRY_FRIEND_RECOMMENDATION_ADD: 'telemetryFriendRecommendationAdd',
            TELEMETRY_FRIEND_RECOMMENDATION_DISMISS: 'telemetryFriendRecommendationDismiss',
            TELEMETRY_FRIEND_RECOMMENDATION_MORE_INFO: 'telemetryFriendRecommendationMoreInfo',

            // Game Library Context Menu
            TELEMETRY_GAMELIBRARY_CONTEXTMENU: 'telemetryGameLibraryContextMenu',

            // Home Tile DOM loaded
            TELEMETRY_ELEMENT_DOM_LOADED: 'telemetryElementDomLoaded',

            // OGD Cog Menu
            TELEMETRY_OGD_COGMENU: 'telemetryOGDCogMenu',

            // Gifting
            TELEMETRY_GIFTING_NO_SEARCH_RESULT: 'telemetryGiftingNoSearchResults',
            TELEMETRY_GIFTING_START_FLOW: 'telemetryGiftingStartFlow',
            TELEMETRY_GIFTING_MESSAGE_PAGE: 'telemetryGiftingMessagePage',

            // Home Video Tiles Events
            TELEMETRY_TILE_VIDEO_STARTED: 'telemetryHometileVideoStarted',
            TELEMETRY_TILE_VIDEO_ENDED: 'telemetryHometileVideoEnded'
        },

        trackerTypes = {
            TRACKER_MARKETING: 'TRACKER_MARKETING',
            TRACKER_DEV: 'TRACKER_DEV'
        };

    /* jshint camelcase: false */

    /**
     * A transaction item object that can be sent in a transaction event to both GA and PIN.
     *
     * See also {@link sendTransactionEvent}
     *
     * @typedef {Object} TransactionItem
     * @type {Object}
     *
     * @property {string} id - The Item ID, sent to both GA and PIN.
     * @property {string} gaCategory - The category sent to GA as the ItemData.category.
     * @property {string} pinCategory - The category sent to PIN as the asset item category.
     * @property {string} name - The item name, sent to both GA and PIN.
     * @property {float} price - The price of the item, in the currency units specified in {@link sendTransactionEvent}.
     * @property {string} revenueModel - This is the receipt.products[i].type value
     */
    function TransactionItem(id, gaCategory, pinCategory, name, price, revenueModel) {
        this.id = id;
        this.gaCategory = gaCategory;
        this.pinCategory = pinCategory;
        this.name = name;
        this.price = price;
        this.revenueModel = revenueModel;
    }

    /**
     * Fires an event to send transaction telemetry to both GA and PIN.  Not all of the fields and properties of the
     * transactionItems are applicable to both, and so will only be sent to the appropriate service.
     *
     * @param {string} transactionId - The transaction ID sent to both GA and PIN.
     * @param {string} storeId - The Store ID to be sent to both GA and PIN.
     * @param {string} currency - The ISO 4217 currency code.
     * @param {float} revenue - The total revenue to be recorded in GA, in currency units specified by {@link currency}.
     * @param {float} tax - The tax amount to be sent to both GA and PIN, in currency units specified by {@link currency}.
     * @param {TransactionItem[]} transactionItems - The items that were purchased.
     * @method
     * @example
     *
     * // Create a transaction item.
     * var items = [];
     * items.push(new TransactionItem('OFB_EAST:1234', 'cat', 'cat', 'Super Mario Brothers', 49.95, 'full_game');
     *
     * // Send the transaction.
     * sendTransactionEvent('1234', 'web', 'USD', 49.95, 5, items);
     */
    function sendTransactionEvent(transactionId, storeId, currency, revenue, tax, transactionItems) {
        myEvents.fire(publicEventEnums.TELEMETRY_TRANSACTION_EVENT, transactionId, storeId, currency, revenue, tax, transactionItems);
    }

    /**
     * Fires a telemetry event to send to both GA and PIN.
     *
     * @param {string} trackerType - The tracker to send to, one of either
     *              'TRACKER_MARKETING' or 'TRACKER_DEV'.
     *
     * @param {string} eventCategory - The GA eventCategory field, typically the
     *              object interacted with, e.g., 'video'.  This is used as the event name
     *              for sending a PIN event..  The event tags will be prefixed with 'origin_'
     *
     * @param {string} eventAction - The GA eventAction field, typically the type of
     *              interaction (e.g. 'play')
     *
     * @param {string} eventLabel - The GA eventLabel field, typically useful for
     *              categorizing events (e.g. 'Fall Campaign')
     *
     * @param {Object} pinFieldsObject - Additional fields to transmit, packed as an
     *              object.  These are passed on to PIN verbatim, and on to GA via the
     *              packParamsAsDimension() function, which extracts only those fields
     *              that are relevant to GA as 'dimensions'.  This ultimately becomes the
     *              fieldsObject passed to GA.
     *
     * @param {number} eventValue - The GA eventValue field, typically a numeric. Optional
     *              value associated with the event (e.g., 42)
     */
    function sendTelemetryEvent(trackerType, eventCategory, eventAction, eventLabel, pinFieldsObject, eventValue, additionalPinParams) {
        myEvents.fire(publicEventEnums.TELEMETRY_EVENT, trackerType, eventCategory, eventAction, eventLabel, pinFieldsObject, eventValue, additionalPinParams);
    }

    /**
     * Fires a telemetry event to send to both GA and PIN.
     *
     * @param {string} trackerType - The tracker to send to, one of either
     *              'TRACKER_MARKETING' or 'TRACKER_DEV'.
     *
     * @param {string} eventCategory - The PIN event name from PIN taxonomy.
     *              Ex. 'login', 'boot_start'
     *
     * @param {string} eventAction - The GA eventAction field, typically the type of
     *              interaction (e.g. 'play')
     *
     * @param {string} eventLabel - The GA eventLabel field, typically useful for
     *              categorizing events (e.g. 'Fall Campaign')
     *
     * @param {Object} pinFieldsObject - Additional fields to transmit, packed as an
     *              object.  These are passed on to PIN verbatim, and on to GA via the
     *              packParamsAsDimension() function, which extracts only those fields
     *              that are relevant to GA as 'dimensions'.  This ultimately becomes the
     *              fieldsObject passed to GA.
     *
     * @param {number} eventValue - The GA eventValue field, typically a numeric. Optional
     *              value associated with the event (e.g., 42)
     */
    function sendStandardPinEvent(trackerType, eventCategory, eventAction, eventLabel, pinFieldsObject, eventValue) {
        myEvents.fire(publicEventEnums.TELEMETRY_PIN_EVENT, trackerType, eventCategory, eventAction, eventLabel, pinFieldsObject, eventValue);
    }


    /**
     * Fire dom loaded telemetry event
     *
     * @method
     */
    function sendDomLoadedTelemetryEvent() {
         myEvents.fire(publicEventEnums.TELEMETRY_ELEMENT_DOM_LOADED); 
    }

    /**
     * Send login/logout telemetry event
     *
     * @param {string} action - "login" or "logout"
     * @param {string} type - "SPA" or "nucleus"
     * @param {string} status - "success", "error"
     * @param {string} statuscode - "normal"
     * @method
     */
    function sendLoginEvent(action, type, status, statuscode) {
        /*jshint camelcase:false */
        // use standard PIN event - login
        sendStandardPinEvent(trackerTypes.TRACKER_MARKETING, action, type, status, {
            'status': status,
            'type': type,
            'status_code':statuscode
        });
        /*jshint camelcase:true */
    }

    function sendErrorEvent(errMessage, errDescription, toPINdetail) {
        //use custom event
        sendTelemetryEvent(trackerTypes.TRACKER_DEV, 'error', errMessage, errDescription, toPINdetail);
    }

    function sendPerformanceTimerEvent(area, startTime, endTime, duration, extraDetail) {
        myEvents.fire(publicEventEnums.TELEMETRY_PERFORMANCE_TIMER, area, startTime, endTime, duration, extraDetail);
    }

    function sendMarketingEvent(category, action, label, value, params, additionalPinParams) {
        sendTelemetryEvent(trackerTypes.TRACKER_MARKETING, category, action, label, params, value, additionalPinParams);
    }

    function sendPageView(page, title, params) {
        myEvents.fire(publicEventEnums.TELEMETRY_PAGEVIEW, page, title, params);
    }

    /**
     * send client action
     * @method
     * @param {string} action
     * @param {string} target
     */
    function sendClientAction(action, target) {
        myEvents.fire(publicEventEnums.TELEMETRY_CLIENT_ACTION, action, target);
    }

    /**
     * set custom dimension
     * @method
     * @param {number} dimension slot
     * @param {string} dimension data
     */
    function setCustomDimension(dimension, data) {
        myEvents.fire(publicEventEnums.TELEMETRY_CUSTOM_DIMENSION, dimension, data);
    }

    utils.mix(myEvents, publicEventEnums);


    return /**@ lends Origin.module:telemetry */ {

        events: myEvents,
        trackerTypes: trackerTypes,

        /**
         *  Creates a transaction item object that can be sent in a transaction event to both GA and PIN.
         *
         * See also {@link sendTransactionEvent}
         *
         * @param {string} id - The Item ID, sent to both GA and PIN.
         * @param {string} gaCategory - The category sent to GA as the ItemData.category.
         * @param {string} pinCategory - The category sent to PIN as the asset item category.
         * @param {string} name - The item name, sent to both GA and PIN.
         * @param {float} price - The price of the item, in the currency units specified in {@link sendTransactionEvent}.
         * @param {string} revenueModel - The PIN revenue model: Must be one of the following: full_game, pdlc, mtx, virtual
         * @constructor
         */
        TransactionItem: TransactionItem,

        /**
         * Fires an event to send transaction telemetry to both GA and PIN.  Not all of the fields and properties of the
         * transactionItems are applicable to both, and so will only be sent to the appropriate service.
         *
         * @param {string} transactionId - The transaction ID sent to both GA and PIN.
         * @param {string} storeId - The Store ID to be sent to both GA and PIN.
         * @param {string} currency - The ISO 4217 currency code.
         * @param {float} revenue - The total revenue to be recorded in GA, in currency units specified by {@link currency}.
         * @param {float} tax - The tax amount to be sent to both GA and PIN, in currency units specified by {@link currency}.
         * @param {TransactionItem[]} transactionItems - The items that were purchased.
         * @method
         * @example
         *
         * // Create a transaction item.
         * var items = [];
         * items.push(new TransactionItem('OFB_EAST:1234', 'cat', 'cat', 'Super Mario Brothers', 49.95, 'full_game');
         *
         * // Send the transaction.
         * sendTransactionEvent('1234', 'web', 'USD', 49.95, 5, items);
         */
        sendTransactionEvent: sendTransactionEvent,

        /**
         * fires an event to send telemetry with origin_ prefix on the event name
         * passes along arguments in the triggered event
         *
         * @method
         */
        sendTelemetryEvent: sendTelemetryEvent,

        /**
         * fires an event to send telemetry
         * passes along arguments in the triggered event
         *
         * @method
         * @param {string} trackerType
         * @param {string} action
         * @param {string} type
         * @param {string} status
         * @param {string} statuscode
         */
        sendStandardPinEvent: sendStandardPinEvent,

        /**
         * Fire dom loaded telemetry event
         *
         * @method
         */
        sendDomLoadedTelemetryEvent: sendDomLoadedTelemetryEvent,

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
         * fires a performance timer event to dev tracker
         * @method
         * @param {string} area
         * @param {float} startTime
         * @param {float} endTIme
         * @param {float} duration
         */
        sendPerformanceTimerEvent: sendPerformanceTimerEvent,

        /**
         * fires an event to the marketing tracker
         * @method
         * @param {string} category event category, e.g. click
         * @param {string} action
         * @param {string} label
         * @param {integer} value GA integer value
         * @param {object} params param object, any additional data
         */
        sendMarketingEvent: sendMarketingEvent,

        /**
         * sends a page view event to marketing tracker
         * @method
         * @param {string} page
         * @param {object} params
         */
        sendPageView: sendPageView,

        /**
         * send client action
         * @method
         * @param {string} action
         * @param {string} target
         */
        sendClientAction: sendClientAction,

        /**
         * set custom dimenion
         * @method
         * @param {number} dimension slot
         * @param {string} dimension data
         */
        setCustomDimension: setCustomDimension
    };
});
