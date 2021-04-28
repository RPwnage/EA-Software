/* TelemetryFactory
 *
 * manages sending telemetry for the app
 * @file factories/telemetry.js
 */
 /* globals ga */

(function() {
    'use strict';

    function TelemetryFactory(APP_VERSION, APPCONFIG, $rootScope, $document, $cookies, $state, $location, LogFactory, SubscriptionFactory, ComponentsConfigHelper, AppCommFactory, GamesClientFactory, GamesDataFactory, LocalStorageFactory, Idle, UrlHelper, ExperimentFactory, ObjectHelperFactory) {

        var DEFAULT_NUCLEUS_ID = '0',
            ORIGINX_MARKETING_TRACKING_ID = 'UA-49146401-5',
            ORIGINX_DEV_TRACKING_ID = 'UA-49146401-3',
            ORIGINX_TID_CLIENT = '196775',
            ORIGINX_TID_WEB = '196775',
            ORIGINX_PIDT = 'origin_web',
            ORIGINX_EVENT_TYPE = 'client_origin_x',
            ORIGIN_PIN_EVENT_CATEGORY_PREFIX = 'origin',
            TRACKER_NAME_MARKETING = '',
            ORIGIN_X_SPA = 'originXspa',
            TRACKER_NAME_DEV = ORIGIN_X_SPA + '.',
            STORE_ID_DEFAULT = 'web',
            GA_SEND = 'send',
            GA_TIMING = 'timing',
            INITIAL_LOAD_CATEGORY = 'InitialLoad',
            PAGE_LOAD_THRESHOLD = 1000, // time for the page DOM to be idle before sending spa page load performance

            PIN_BUFFERING_DELAY_MILLISECONDS = 1000, // The delay for buffering PIN events.
            PIN_BUFFER_MAX_EVENTS = 10000, // The maximum number of events allowed to be enqueued in the buffer before events start to be dropped.
            TELEMETRY_BUFFER_MAX = 500, // max number of events per POST to PIN.  PIN has a limit of 1meg per POST

            OPERATION_DOWNLOAD = 'download', // install flow operation types
            OPERATION_UPDATE = 'update',
            OPERATION_REPAIR = 'repair',

            step = 0,  //Telemetry Step counter
            pageViewCount = 0,  // Track the number of times we call sendTelemetryPageView()
                //Need a way to create and store a sessionID
            sessionId = '0',  //GA automatically tracks a session ID in a cookie. You can tune the rules about what a session is.
                              // Usually all interactions with less than a 30 min break.
            clientId = '',
            currentDate = new Date(),
            isEmbeddedBrowser = false,
            platform = 'web',
            pinTid = ORIGINX_TID_WEB,
            telemetryBuffer = [],
            releaseType = 'prod',
            currentContentGroup = '',
            revenueModelType = {
                FULL_GAME: 'full_game',
                DLC: 'dlc',
                MICROTRANSACTION: 'mtx'
            },
            offerIdType = {
                BASE_GAME: 'Base Game',
                EXTRA_CONTENT: 'Extra Content',
            },
            settings = {
                TelemetryXML: false
            },
            sourceType = {
                ITO: 'ITO',
                GAME_FINISHED: 'GameFinished',
                CLIENT_MENU: 'ClientMenu',
                CLIENT_START: 'ClientStart'
            },
            firstPageLoad = true,
            mutationObserver = null,
            timeoutHandle = null,
            lastOfferId = '',
            internalCampaign = '',
            gameState = {},
            lastAction = {},
            playClickedTS = 0,
            downloadClickedTS = 0,
            session = {
                startTime: Date.now(),
                endTIme: 0,
                id: Origin.utils.generateUUID()
            },
            downloadPhaseToStatusMap = {
                'READYTOSTART': 'origin_readytostart',
                'PAUSED': 'origin_paused',
                'PAUSING': 'pause',
                'INITIALIZING': 'origin_initializing',
                'ENQUEUED': 'origin_enqueued',
                'PREPARING': 'origin_preparing',
                'RESUMING': 'resume',
                'FINALIZING': 'origin_finalizing',
                'CANCELLING': 'cancel',
                'INSTALLING': 'origin_installing',
                'READYTOINSTALL': 'origin_readytoinstall',
                'COMPLETED': 'finish',
                'RUNNING': 'start'
            };

        var trackerName = {};
        trackerName[Origin.telemetry.trackerTypes.TRACKER_MARKETING] = TRACKER_NAME_MARKETING;
        trackerName[Origin.telemetry.trackerTypes.TRACKER_DEV] = TRACKER_NAME_DEV;


        function setTelemetryXML(val) {
            settings.TelemetryXML = (val === true);
        }

        function isEmbeddedBrowserAndOffline() {
            if (isEmbeddedBrowser && !Origin.client.onlineStatus.isOnline()) {
                return true;
            }
            return false;
        }

        function isTelemetryLogEnabled() {
            return $cookies.get('telemetry') || isEmbeddedBrowser && settings.TelemetryXML;
        }

        function telemetryLog() {
            if (isTelemetryLogEnabled()) {
                LogFactory.log.apply(null, arguments);
            }
        }

        function httpRequest(endPoint, config) {
            var HTTP_REQUEST_TIMEOUT = 60000,   //recommended timeout from PIN docs
                errResponse;

            if (endPoint === '') {
                errResponse.status = -1;
                return Promise.reject(errResponse);
            } else {
                config.timeout = HTTP_REQUEST_TIMEOUT;
                return Origin.dataManager.enQueue(endPoint, config);
            }
        }

        function handlePINerror(error) {
            //TODO: need to set up a retry if 500 or 503
            //DO NOT use LogFactory.error here; otherwise we'll get recursive calls to telemetr to log the error
            if (error.status === 500 || error.status === 503) {
                telemetryLog('httpStatus = ', error.status, ', need to retry');
            } else {
                telemetryLog('unexpected error from PIN:', error);
            }
        }

        function setPinPlatform(os) {
            //allowed values
            //xbox_one
            //ps4
            //xbox_360
            //ps3
            //pc
            //iOS
            //win
            //android
            //mac
            //other

            platform = 'web';

            if (isEmbeddedBrowser) {
                if (os === 'PCWIN') {
                    platform = 'pc';
                } else if (os === 'MAC'){
                    platform = 'mac';
                }
            }

        }

        function delay(time) {
            return new Promise(function(fulfill) {
                setTimeout(fulfill, time);
            });
        }

        function transmitTelemetryBuffer() {
            if (!isEmbeddedBrowserAndOffline()) {
                if (telemetryBuffer.length > 0) {
                    var sendTelemetryBuffer = _.slice(telemetryBuffer, 0, TELEMETRY_BUFFER_MAX);
                    telemetryBuffer = _.slice(telemetryBuffer, TELEMETRY_BUFFER_MAX);
                    var ts = new Date().toISOString(),
                        endPoint = APPCONFIG.urls.pinRiverEndpoint,
                        config = {
                            atype: 'POST',
                            headers: [
                                { label: 'Content-Type', val: 'application/json' },
                                { label: 'x-ea-game-id', val: 'OriginSPA' },
                                { label: 'x-ea-game-id-type', val: 'client' },
                                { label: 'x-ea-app-type', val: 'WEBTRACKING' },
                                { label: 'x-ea-taxv', val: '1.1' }
                            ],
                            body: JSON.stringify({
                                'taxv': '1.1',
                                'tidt': 'projectid',
                                'tid': pinTid,
                                'rel': releaseType,
                                'v': APP_VERSION,  //Origin Version
                                'ts_post': ts,
                                'sid': sessionId, //Session ID we need to make one of these
                                'plat': platform,
                                'et': ORIGINX_EVENT_TYPE,
                                //'clientIP': '10.10.28.1', //Probably shouldn't send this.
                                'loc': Origin.locale.locale(),
                                'events': sendTelemetryBuffer
                            })
                        };
                    httpRequest(endPoint, config)
                        .then(transmitTelemetryBuffer)
                        .catch(handlePINerror);
                }
            }
        }

        /**
         * Fetch a client id value that's cached.  Use localstorage to cache the value otherwise create a new UUID
         *
         * @return {string} UUID
         */
        function getClientId() {
            var pid = LocalStorageFactory.get('origin.clientid', null, true);
            if (!pid) {
                pid = Origin.utils.generateUUID();
                LocalStorageFactory.set('origin.clientid', pid, true);
            }
            return pid;
        }

        /**
         * Queue a telemetry event to be sent to the PIN-River service.
         *
         * @param {string} tracker - the tracker name.  Either 'TRACKER_MARKETING' or
         *              'TRACKER_DEV'
         * @param {object} event additional data to send to PIN
         */
        function queuePinEvent(tracker, eventName, event, additionalParameters) {

            var ts = new Date().toISOString();

            event.core = {};

            event.core.en = eventName;

            event.core.pidt = ORIGINX_PIDT;

            event.core.tid = pinTid;

            //advance step
            step++;

            //add more stuff to coreparams
            /* jshint camelcase: false */
            event.core.ts_event = ts;
            /* jshint camelcase: true */
            event.core.sid = sessionId;
            event.core.s = step;

            event.core.pid = clientId;   //use the clientID as pid for both anonymous and logged in and use pidm to specify nucleusId if logged in

            $.extend(event.core, additionalParameters);

            var inExperimentStr = ExperimentFactory.getUserCustomDimension();
            if (!_.isEmpty(inExperimentStr)) {
                event.core.exid = inExperimentStr;
            }

            //if user is logged in we need to get their nucleus Id and date of birth
            //we're purposely not using AuthFactory in case this code needs to be called from the bootstrapper
            if (Origin.user && Origin.auth.isLoggedIn()) {
                event.core.pidm = {'nucleus': Origin.user.userPid()};
                event.core.dob = Origin.user.dob().substring(0, 7); // first 7 chars are "yyyy-mm", which is what PIN wants
            } else {
                event.core.pidm = {'nucleus': DEFAULT_NUCLEUS_ID};
            }

            /* jshint camelcase: false */
            event.install_type = 'other';
            /* jshint camelcase: true */

            // If there is no telemetry queued for transmission,
            if (telemetryBuffer.length === 0) {

                // Schedule telemetry transmission after the max time delay.
                delay(PIN_BUFFERING_DELAY_MILLISECONDS).then(transmitTelemetryBuffer);
            }


            // If there is available room in the buffer,
            if (telemetryBuffer.length < PIN_BUFFER_MAX_EVENTS) {
                // we don't want to stringify if logging it not enabled
                if (isTelemetryLogEnabled()) {
                    telemetryLog('telemetry:', 'queuePinEvent(', JSON.stringify(event, null, 2), ')');
                }
                // Enqueue the event for transmission.
                telemetryBuffer.push(event);
            }
            else {
                telemetryLog('telemetry:', 'queuePinEvent() ERROR: queue failed, buffer is full');
            }
        }

        function setGAContentGroup(stateName, isCheckout) {
            var contentGroup;
            if (isCheckout) {
                contentGroup = 'Checkout';
            } else {
                if (_.startsWith(stateName, 'app.game_gamelibrary.ogd')) {
                    contentGroup = 'OGD';
                } else if (_.startsWith(stateName, 'app.store.wrapper.pdp') || _.startsWith(stateName, 'app.store.wrapper.addon')) {
                    contentGroup = 'PDP';
                } else {
                    contentGroup = 'Default';
                }
            }
            if (currentContentGroup !== contentGroup) {
                currentContentGroup = contentGroup;
                ga('set', 'contentGroup1', contentGroup);
            }
        }

        /**
         * Go through the params list and pick out the ones that we want to send to GA and map them to the custom dimension or metric.
         *
         * @param {object} params
         */
        function packParamsAsDimension(params) {

            // The big issue with this is that GA only allows pre-defined parameters.  Adding extra parameters need to be
            // custom dimensions or metrics so we maintain a mapping from PIN to GA for the parameters we care about in GA.
            var gaParamFilter = {
                'status': 'dimension21',
                'type' : 'dimension22',
                'status_code' : 'dimension23',

                //mutually exclusive from above;params used by performance timer event
                'strt': 'dimension21',
                'endt': 'dimension22',
                'time': 'dimension23'
            };

            var outParams = {};

            _.forEach(params, function(value, key) {
                if (key in gaParamFilter && value) {
                    outParams[gaParamFilter[key]] = value.toString();
                } else if (value) {
                    outParams[key] = value.toString();
                }
            });

            var subscriptionStatus = SubscriptionFactory.getStatus();
            // Make sure we have the subs data already and we never got offerid
            if (SubscriptionFactory.getSubscriptionInfoRetrieved() &&
                SubscriptionFactory.getUserLastSubscriptionOfferId() === undefined) {
                subscriptionStatus = 'NEVERSUBSCRIBED';
            }

            // update GA custom dimensions
            outParams.dimension1 = Origin.user.userPid().toString();
            outParams.dimension2 = Origin.user.userPid().length === 0 ? 'Logged Out' : 'Logged In';
            outParams.dimension3 = subscriptionStatus;
            outParams.dimension4 = Origin.locale.languageCode();
            outParams.dimension5 = isEmbeddedBrowser ? Origin.utils.os() : 'WEB';
            outParams.dimension6 = document.referrer;
            if (isEmbeddedBrowser) {
                outParams.dimension7 = Origin.client.info.version();
            }
            var source = $location.search().source;
            if (source) {
                outParams.dimension8 = source;
                if (source === sourceType.GAME_FINISHED || source === sourceType.ITO) {
                    outParams.dimension9 = lastOfferId;
                    lastOfferId = '';
                }
            }
            outParams.dimension10 = APP_VERSION;
            outParams.dimension11 = internalCampaign;
            outParams.dimension13 = Origin.locale.threeLetterCountryCode(); // storefront

            //check and see if the Experimentation custom dimension is set, if so, then add it
            var experimentCD = ExperimentFactory.getUserCustomDimension();
            if (!_.isEmpty(experimentCD)) {
                outParams.dimension14 = experimentCD;
            }
            return outParams;
        }

        function isDevTelemetryEnabled() {
            if (APPCONFIG.tealium.disableGaDevTelemetry === 'true') {
                return false;
            }
            if (location.hostname === 'origin.com' || location.hostname === 'www.origin.com') {
                return false;
            }
            return true;
        }

        function sendEventToGA(tracker, category, action, label, step, gaParams) {
            // don't send to GA if we're offline in the embeddded browser, or if this is dev tracker telemetry and dev telemetry is disabled for GA
            if (isEmbeddedBrowserAndOffline() || ((tracker === _.get(trackerName, Origin.telemetry.trackerTypes.TRACKER_DEV)) && !isDevTelemetryEnabled())) {
                return;
            }

            ga(tracker + 'send', 'event', category, action, label, step, gaParams);
            //eventually may want to queue this request until we're back online
        }

        function sendEventInternal(trackerType, gaEventCategory, pinEventCategory, eventAction, eventLabel, pinFieldsObject, eventValue, additionalPinParams) {
            // Prepare to extract arguments and send events.
            var fieldsObject,
                tracker;

            fieldsObject = packParamsAsDimension(pinFieldsObject);

            tracker = trackerName[trackerType];

            if (typeof pinFieldsObject !== 'object') {
                pinFieldsObject = {};
            }
            pinFieldsObject.eventCategory = gaEventCategory;
            pinFieldsObject.eventAction = eventAction;
            pinFieldsObject.eventLabel = eventLabel;
            pinFieldsObject.eventValue = eventValue;

            // update the content grouping for GA
            setGAContentGroup($state.current.name, pinFieldsObject.isCheckout);

            // We are currently separating out the telemetry events: we will send GA events only to GA, and send PIN events only to PIN.
            // Previously, we had been sending both kinds of events to both places (with some exceptions).  We can distinguish GA events from
            // PIN events by the pinEventCategory.  GA events use a prefix of 'origin_' in the pinEventCategory.

            // send GA event
            if (_.startsWith(pinEventCategory, 'origin_')) {
                sendEventToGA(tracker, gaEventCategory, eventAction, eventLabel, eventValue, fieldsObject);
            }
            // send PIN event
            else {
                queuePinEvent(trackerType, pinEventCategory, pinFieldsObject, additionalPinParams);
            }
        }

        /*
         * Send an event to both Google Analytics and EADP PIN.  This function sends
         * an 'event' hitType to Google Analytics, recording the category, action, label,
         * value, and any other applicable parameters; it sends a PIN taxonomy EADP
         * event as well, consisting of the category as the event name, the specified
         * parameters, and the value.
         *
         * @param {string} trackerType - The tracker to send to, one of either
         *              'TRACKER_MARKETING' or 'TRACKER_DEV'.
         *
         * @param {string} eventCategory - The Ga eventCategory field, typically the
         *              object interacted with, e.g., 'video'.  This is used as the event name
         *              for sending a PIN event..
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
            sendEventInternal(trackerType, eventCategory, eventCategory, eventAction, eventLabel, pinFieldsObject, eventValue);
        }

        /**
         * Convert a google analytics category name to a PIN category name
         * Ensure it's a string and follows the convention: origin_{event_name_...}
         * @param  {mixed} text the input to be sanitized
         * @return {String} the sanitized string
         */
        function convertGaCategoryToPinCategory(text) {
            var eventName = _.snakeCase(text + '');

            return [ORIGIN_PIN_EVENT_CATEGORY_PREFIX, eventName].join('_');
        }

        /*
         * Send an event to both Google Analytics and EADP PIN.  This function sends
         * an 'event' hitType to Google Analytics, recording the category, action, label,
         * value, and any other applicable parameters; it sends a PIN taxonomy EADP
         * event as well, consisting of the category as the event name, the specified
         * parameters, and the value.
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
        function sendTelemetryEvent(trackerType, gaEventCategory, eventAction, eventLabel, pinFieldsObject, eventValue, additionalPinParams) {
            sendEventInternal(trackerType, gaEventCategory, convertGaCategoryToPinCategory(gaEventCategory), eventAction, eventLabel, pinFieldsObject, eventValue, additionalPinParams);
        }

        /*
         * Set a Custom Dimension to GA only
         *
         * @param {number} dimensionSlot - Slot to assign data to
         * @param {string} data - Data
         */
        function setCustomDimension(dimensionSlot, data) {
            sendCommandToGA('set', 'dimension' + dimensionSlot, data);
        }

        function sendCommandToGA(cmd, param, fieldsObject) {
            if (!isEmbeddedBrowserAndOffline()) {
                ga(cmd, param, fieldsObject);
            }
            //eventually may want to queue this request until we're back online
        }

        /**
         * sends a page view event
         * WARNING: this function should only be invoked through the event system (triggered by nav event),
         * and by setInitialGALandingPage()...to keep the pageViewCount var valid.
         *
         * @param {string} page the ui-route uri
         * @param {string}  title ui-route name
         * @param {object} params extra data
         */
        function sendTelemetryPageView(page, title, params) {
            ++pageViewCount;

            //assume marketing
            var pageObj = {};

            // remove query string except Search query
            var searchString = UrlHelper.extractSearchParamByName('searchString', page);
            var query = searchString ? '?searchString=' + searchString : '';
            var path = page.split('?')[0] + query;
            pageObj.page = path;
            pageObj.title = title;

            setGAContentGroup(params.toid, false);

            //set it so all subsequent events will be from this "page"
            sendCommandToGA('set', pageObj);

            var fieldsObject = packParamsAsDimension({});

            // we have to send a startup pageView event to GA to set our initial landing page,
            // so we don't want to also send the first 'real' pageView event, since it will just be a duplicate.
            if (pageViewCount !== 2) {
                sendCommandToGA('send', 'pageView', fieldsObject);
            }

            if (typeof params === 'undefined') {
                params = {};
            }
            params.type = 'web_page';
            params.pgid = params.toid;

            // don't send the first pageView event to PIN.  it will be a startup pageView event that is only needed
            // for GA, to set the initial landing page.  it will not contain a proper state, so we're better off
            // ignoring it for PIN.
            if (pageViewCount !== 1) {
                queuePinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'page_view', params);
            }
        }

        /**
         * Sends a transaction event to both GA and PIN.  Not all of the fields and properties of the
         * transactionItems are applicable to both, and so will only be sent to the appropriate service.
         *
         * @param {string} transactionId - The transaction ID sent to both GA and PIN.
         * @param {string} storeId - The Store ID to be sent to both GA and PIN.  Can be set to null to auto detect the store type.
         * @param {string} currency - The ISO 4217 currency code.
         * @param {float} revenue - The total revenue to be recorded in GA, in currency units specified by {@link currency}.
         * @param {float} tax - The tax amount to be sent to both GA and PIN, in currency units specified by {@link currency}.
         * @param {TransactionItem[]} transactionItems - The items that were purchased.
         */
        function sendTransactionEvent(transactionId, storeId, currency, revenue, tax, transactionItems) {

            var pinTransaction,
                revenueModel = revenueModelType.FULL_GAME;

            if (_.isArray(transactionItems) && transactionItems.length > 0) {
                var revenueModels = [];
                // map offer type in transactionItems to revenue model
                _.forEach(transactionItems, function(transactionItem) {
                    if (transactionItem.revenueModel === offerIdType.BASE_GAME) {
                        revenueModels.push(revenueModelType.FULL_GAME);
                    } else if (transactionItem.revenueModel === offerIdType.EXTRA_CONTENT) {
                        revenueModels.push(revenueModelType.DLC);
                    } else {
                        revenueModels.push(revenueModelType.MICROTRANSACTION);
                    }
                });
                revenueModel = revenueModels.join(',');
            }

            // we override storeId to the appropriate value
            // OIG or ODC
            if (isEmbeddedBrowser) {
                storeId = 'client';
                if (_.startsWith(window.location.pathname, '/oig')) {
                    if (ComponentsConfigHelper.getOIGContextConfig()) {
                        storeId = 'oig';
                    } else {
                        storeId = 'odc';
                    }
                }
            } else {
                if (!storeId) {
                    storeId = STORE_ID_DEFAULT;
                }
            }

            // Create the GA TransactionData object.
            ga('ecommerce:addTransaction', {
                'id': transactionId,
                'affiliation': storeId,
                'revenue': revenue.toString(),
                'shipping': '0',
                'tax': tax,
                'currency': currency
            });

            // Create the PIN transaction object.
            pinTransaction = {
                'txid': transactionId,
                'code': 'Entitlement',
                'type': 'entitlement_purchase',
                'revenue_model': revenueModel,
                'status': 'complete',
                'status_code': '',
                'party1id': {'nuc_id': Origin.user.userPid()},
                'party2id': storeId,
                'tax': tax,
                'revenue': revenue,
                'asset_out': {'currencies': {'real': []}},
                'asset_in': {'items': {'entitlement': []}}
            };

            // Loop over the items being purchased.
            if (_.isArray(transactionItems)) {
                _.forEach(transactionItems, function (transactionItem) {

                    // Add a GA ItemData object for this item.
                    ga('ecommerce:addItem', {
                        'id': transactionId,
                        'name': transactionItem.name,
                        'sku': transactionItem.id,
                        'category': transactionItem.gaCategory,
                        'price': transactionItem.price.toString(),
                        'quantity': 1
                    });

                    // Send the asset_out event
                    /* jshint camelcase: false */
                    var assetOut = {
                        'category': transactionItem.pinCategory,
                        'id': transactionItem.id,
                        'name': currency, // It is intentional that 'name' is tracking 'currency'
                        'quantity': transactionItem.price // It is intentional that 'quantity' is tracking 'transactionItem.price'
                    };
                    pinTransaction.asset_out.currencies.real.push(assetOut);

                    // Add a PIN asset item for this item.
                    var assetIn = {
                        'category': transactionItem.pinCategory,
                        'id': transactionItem.id,
                        'name': transactionItem.name,
                        'quantity': transactionItem.price // It is intentional that 'quantity' is tracking 'transactionItem.price'
                    };
                    if (transactionItem.months) {
                        assetIn.months = transactionItem.months;
                    }
                    pinTransaction.asset_in.items.entitlement.push(assetIn);
                    /* jshint camelcase: true */
                });
            }

            // Send the complete PIN transaction event.
            queuePinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'transaction', pinTransaction);

            // Send the complete GA ecommerce event.
            ga('ecommerce:send');
        }

        /**
         * Client action received
         * @param {string} eventargs[0] action
         * @param {string} eventargs[1] target
         */
        function sendClientActionEvent(action, target) {
            telemetryLog('sendClientActionEvent(action = ' + action + ' target = ' + target + ')');
            // save the last action
            if (!lastAction) {
                lastAction = {};
            }

            if (action === 'play') {
                playClickedTS = Date.now();

                if (GamesDataFactory.isEarlyAccess(target)) {
                    sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Game Library', 'Launch Early Access Trial', target);
                } else if (GamesDataFactory.isTrial(target)) {
                    sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Game Library', 'Launch Trial', target);
                } else {
                    sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Game Library', 'Launch Game', target);
                }
            }
            var downloadActions = [
                'startDownload',
                'cancelDownload',
                'pauseDownload',
                'resumeDownload',
                'installUpdate',
                'pauseUpdate',
                'resumeUpdate',
                'cancelUpdate',
                'pauseRepair',
                'resumeRepair'
                ];
            if (_.contains(downloadActions, action)) {
                downloadClickedTS = Date.now();
            }
            lastAction.action = action;
            lastAction.target = target;
            lastAction.ts = Date.now();
        }

        /**
         * Send a boot start event.  Should be called to notifiy the app boot from angular is done.
         */
        function sendBootStartEvent() {
            var source = isEmbeddedBrowser ? 'client' : 'web';
            Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'boot_start', source, 'success', {
                'status': 'success',
                'source': source,
                'status_code': 'success'
            });
        }

        /**
         * Send performance timer event
         * @param {string} area - Name of the performance timer
         * @param {number} startTime - start timer timestamp in ms
         * @param {number} endTime - end timer timestamp in ms
         * @param {number} duration - difference between start and end in ms
         * @param {object} extraDetail - dictionary
         */
        function sendPerformanceTimerEvent(area, startTime, endTime, duration, extraDetail) {
            var loggedIn = Origin && Origin.auth && Origin.auth.isLoggedIn() || false;
            var myOfferIds = GamesDataFactory.baseEntitlementOfferIdList();
            var myOfferIdCount = _.size(myOfferIds);
            var page = $state.current.name || 'nopage';
            var automationValue = $location.search().automation || 'none';
            // truncate the string if the value is too large
            if (automationValue.length > 32) {
                automationValue.length = 32;
            }

            telemetryLog('[TELEMETRY] ' + area + ' dt: ' + duration + ', page: ' + page);
            var detail = {
                'area': area,
                'strt': startTime,
                'endt': endTime,
                'time': duration,
                'loggedIn': loggedIn ? '1' : '0',
                'ownedGameCount': myOfferIdCount,
                'userAgent': window.navigator.userAgent,
                'page': page,
                'automation': automationValue
            };
            _.merge(detail, extraDetail);
            sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_DEV, 'perf', area, 'time', detail);
        }

        /**
         * get the performance measure for a measure name
         * @param  {string} measure the measure name
         * @return {object}         the peformance timing object
         */
        function getPerformanceMeasure(measure) {
            //getMeasure returns an array of 1
            return _.first(Origin.performance.getMeasure(measure));
        }

        /**
         * builds an object for the performance table
         * @param  {string} initialState the initial state we are trying to route to
         * @param  {object} item         the performance object from the window.performance call
         * @return {object}              an object used to in the display table
         */
        function buildObjectInPerformanceTableFormat(initialState, item) {
            return {
                'initialState': initialState,
                'startTime(ms)': Math.round(item.startTime),
                'duration(ms)': Math.round(item.duration),
                'timeSinceLoad(ms)': Math.round(item.startTime + item.duration)
            };
        }

        /**
         * displays a table in the console with the listed timings
         * @param  {string} toStateName the name of the initial state we are trying to route to
         */
        function showInitialPerformanceTableInConsole(initialState) {
            //list constants related to initiaLoad
            var initialMeasuresConstants = [
                    OriginPerfConstant.measure.START_TO_HEAD,
                    OriginPerfConstant.measure.HEAD_TO_ANGULAR_START,
                    OriginPerfConstant.measure.ANGULAR_START_TO_FIRST_ROUTE,
                    OriginPerfConstant.measure.FIRST_ROUTE_START_TO_PAGE_LOAD,
                ],
                initialMeasures = _.map(initialMeasuresConstants, getPerformanceMeasure),
                tableFn = _.get(window, ['console', 'table']);

            initialMeasures = _.indexBy(initialMeasures, 'name');
            initialMeasures = _.mapValues(initialMeasures, _.partial(buildObjectInPerformanceTableFormat, initialState));

            //if we have a table fn use it
            if (tableFn) {
                tableFn(initialMeasures);
            } else {
                LogFactory.log(initialMeasures);
            }
        }

        /**
         * sends the initial performance timings to GA timing (not GA event)
         * @param  {string} toStateName the name of the initial state we are trying to route to
         */
        function sendInitialPerformanceTelemetry(toStateName) {
            //list constants related to initiaLoad
            var initialMeasuresConstants = [
                OriginPerfConstant.measure.START_TO_ANGULAR_START,
                OriginPerfConstant.measure.START_TO_FIRST_PAGE_LOAD
            ],
            initialMeasures = _.map(initialMeasuresConstants, getPerformanceMeasure);
            //send timing to GA, PIN pending
            _.forEach(initialMeasures, function(item) {
                ga(TRACKER_NAME_DEV + GA_SEND, GA_TIMING, INITIAL_LOAD_CATEGORY, item.name, item.duration, toStateName);
            });
        }

        /**
         * Callback function when ui-route is about to change pages.  Called when a user initiates a navigation.
         */
        function onPageViewStart() {
            Origin.performance.clearMarker(OriginPerfConstant.markers.ROUTE_CHANGE_START);
            Origin.performance.setMarker(OriginPerfConstant.markers.ROUTE_CHANGE_START);
            Origin.performance.setMeasure(OriginPerfConstant.measure.ANGULAR_START_TO_FIRST_ROUTE, OriginPerfConstant.markers.ANGULAR_BOOTSTRAP_START, OriginPerfConstant.markers.ROUTE_CHANGE_START);
            if (timeoutHandle) {
                clearTimeout(timeoutHandle);
                timeoutHandle = null;
            }
        }

        /**
         * Callback function when ui-route is page change is done.  Also called if an error is received.
         *
         * When the page url change is notified from ui-router, MutationObserver is added to the document.
         * If the DOM changes, the timeout for PAGE_LOAD_THRESHOLD is reset.  The heuristic is when the
         * DOM hasn't changed for a PAGE_LOAD_THRESHOLD time, assume the page has finished loading.
         */
        function onPageViewDone(event, toState) {
            var MutationObserver = window.MutationObserver || window.WebKitMutationObserver || window.MozMutationObserver;
            if (MutationObserver) {
                if (mutationObserver) {
                    mutationObserver.disconnect();
                }

                mutationObserver = new MutationObserver(function() {
                    Origin.performance.clearMarker(OriginPerfConstant.markers.PAGE_LOAD_COMPLETED);
                    Origin.performance.setMarker(OriginPerfConstant.markers.PAGE_LOAD_COMPLETED);
                    if (timeoutHandle) {
                        clearTimeout(timeoutHandle);
                    }
                    timeoutHandle = setTimeout(function() {
                        Origin.performance.clearMeasure(OriginPerfConstant.measure.ROUTE_START_TO_PAGE_LOAD);
                        Origin.performance.setMeasure(OriginPerfConstant.measure.ROUTE_START_TO_PAGE_LOAD, OriginPerfConstant.markers.ROUTE_CHANGE_START, OriginPerfConstant.markers.PAGE_LOAD_COMPLETED);

                        if (firstPageLoad) {
                            firstPageLoad = false;
                            Origin.performance.setMeasure(OriginPerfConstant.measure.FIRST_ROUTE_START_TO_PAGE_LOAD, OriginPerfConstant.markers.ROUTE_CHANGE_START, OriginPerfConstant.markers.PAGE_LOAD_COMPLETED);
                            Origin.performance.setMeasure(OriginPerfConstant.measure.START_TO_FIRST_PAGE_LOAD, OriginPerfConstant.markers.NAVIGATION_START, OriginPerfConstant.markers.PAGE_LOAD_COMPLETED);
                            Origin.performance.setMeasure(OriginPerfConstant.measure.START_TO_ANGULAR_START, OriginPerfConstant.markers.NAVIGATION_START, OriginPerfConstant.markers.ANGULAR_BOOTSTRAP_START);

                            //only want to send in production env with live cms
                            if (APPCONFIG.overrides.env === '' && APPCONFIG.overrides.cmsstage === '') {
                                sendInitialPerformanceTelemetry(toState.name);
                            }
                            //show a table in the console with the performance values, if logging is requested
                            if(APPCONFIG.overrides.showlogging) {
                                showInitialPerformanceTableInConsole(toState.name);
                            }
                        } else {
                            Origin.performance.setMeasure(OriginPerfConstant.measure.ROUTE_START_TO_PAGE_LOAD, OriginPerfConstant.markers.ROUTE_CHANGE_START, OriginPerfConstant.markers.PAGE_LOAD_COMPLETED);
                        }

                        timeoutHandle = null;
                        if (mutationObserver) {
                            mutationObserver.disconnect();
                            mutationObserver = null;
                        }
                    }, PAGE_LOAD_THRESHOLD);
                });
                mutationObserver.observe(document, { childList: true, subtree: true});
            }
        }

        /**
         * Fetch game state from offerId
         * @param {string} offerId - id of game to store the state
         */
        function getGameState(offerId) {
            var info = gameState[offerId];
            if (!info) {
                info = {
                    sessionId: Origin.utils.generateUUID()
                };
                gameState[offerId] = info;
            }

            return info;
        }

        /**
         * Store game state from offerId.  Also add sessionId to track the game state.
         * @param {string} offerId - id of game to store the state
         * @param {object} state - state of the game object to store
         */
        function setGameState(offerId, state) {
            if (gameState.hasOwnProperty(offerId)) {
                state.sessionId = gameState[offerId].sessionId;
            } else {
                state.sessionId = Origin.utils.generateUUID();
            }
            gameState[offerId] = state;
        }

        /**
         * Immediately send (do not queue) a PIN boot_start or boot_end event, for a game launch/exit.
         * These events are not queued up with our regular PIN transmissions because they require custom headers.
         * @param {object} catalogInfo - to retrieve the projectId for the game
         * @param {bool} isStarting - true if the game is launching (not exiting)
         */
        function sendCustomBootStartEnd(catalogInfo, isStarting) {
            var ts = new Date().toISOString(),
                prevGameState = getGameState(catalogInfo.offerId),
                eaGameTidt = catalogInfo.projectNumber ? 'projectid' : 'origin_offerid',
                eaGameTid = catalogInfo.projectNumber ? catalogInfo.projectNumber : catalogInfo.offerId,
                isNonOriginGame = _.startsWith(catalogInfo.offerId, 'NOG_'),
                event = {};
            event.core = {};
            event.core.en = isStarting ? 'boot_start': 'boot_end';
            event.core.pidt = ORIGINX_PIDT;
            /* jshint camelcase: false */
            event.core.ts_event = ts;
            /* jshint camelcase: true */
            event.core.sid = prevGameState.sessionId;
            event.core.s = isStarting ? 1: 2;
            event.core.pid = clientId;   //use the clientID as pid for both anonymous and logged in and use pidm to specify nucleusId if logged in

            //if user is logged in we need to get their nucleus Id and date of birth
            //we're purposely not using AuthFactory in case this code needs to be called from the bootstrapper
            if (Origin.user && Origin.auth.isLoggedIn()) {
                event.core.pidm = {'nucleus': Origin.user.userPid()};
                event.core.dob = Origin.user.dob().substring(0, 7); // first 7 chars are "yyyy-mm", which is what PIN wants
            } else {
                event.core.pidm = {'nucleus': DEFAULT_NUCLEUS_ID};
            }

            /* jshint camelcase: false */
            event.install_type = 'other';
            /* jshint camelcase: true */
            event.eventCategory = isStarting ? 'boot_start': 'boot_end';
            event.eventAction = 'client';
            event.eventLabel = 'success';

            if (isStarting) {
                event.status = 'success';
                event.source = 'normal';
            }
            else {
                /* jshint camelcase: false */
                event.end_reason = 'normal';
                /* jshint camelcase: true */
            }

            if(isNonOriginGame) {
                /* jshint camelcase: false */
                event.app_name = catalogInfo.offerId;
                event.app_id = catalogInfo.offerId;
                /* jshint camelcase: true */
            }

            // we don't want to stringify if logging it not enabled
            if (isTelemetryLogEnabled()) {
                telemetryLog('telemetry:', 'sendPinEvent(', JSON.stringify(event, null, 2), ')');
            }

            var endPoint = APPCONFIG.urls.pinRiverEndpoint,
                config = {
                    atype: 'POST',
                    headers: [
                        { label: 'Content-Type', val: 'application/json' },
                        { label: 'x-ea-game-id', val: 'OriginSPA' },
                        { label: 'x-ea-game-id-type', val: 'client' },
                        { label: 'x-ea-app-type', val: 'WEBTRACKING' },
                        { label: 'x-ea-taxv', val: '1.1' }
                    ],
                    body: JSON.stringify({
                        'taxv': '1.1',
                        'tidt': isNonOriginGame ? 'non_ea_app' : eaGameTidt,
                        'tid': isNonOriginGame ? 'non_ea_app' : eaGameTid,
                        'rel': releaseType,
                        'v': gameState.availableUpdateVersion,
                        'ts_post': ts,
                        'sid': prevGameState.sessionId,
                        'plat': platform,
                        'et': ORIGINX_EVENT_TYPE,
                        'loc': _.get(Origin, 'locale.locale') ? Origin.locale.locale() : '',
                        'events': [event]
                    })
                };
            httpRequest(endPoint, config)
                .catch(handlePINerror);
        }

        /**
         * Send telemetry data when a game is started or stopped
         * @param {boolean} isStarting - true if launching game, false if game ended
         * @param {string} offerId - id of the game
         */
        function sendGameLaunch(isStarting, offerId) {
            var prevGameState = getGameState(offerId);
            var launchSource = '';
            var launchType = isStarting ? 'start' : 'stop';
            var now = Date.now();
            var dt = now - (playClickedTS || 0);

            // figure out if we started from the client or desktop
            if (isStarting) {
                if (dt < 15*1000) {
                    launchSource = 'client';
                } else {
                    launchSource = 'desktop';
                }
            }
            telemetryLog('sendGameLaunch() ' + launchType.toUpperCase() + ' PLAYING: ' + offerId + ' source=' + launchSource);
            var info = {
                gsid: prevGameState.sessionId,
                gidt: _.startsWith(offerId, 'NOG_') ? 'nogsid' : 'offerid',
                gid: offerId,
                gvs: gameState.availableUpdateVersion,
                status: launchType,
                /* jshint camelcase: false */
                launch_source: launchSource
                /* jshint camelcase: true */
            };
            sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'game_launch', launchType, offerId, info);

            var launchAction = isStarting ? 'Launch Game' : 'End Game';
            sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'General', launchAction, offerId);

            // ORPUB-3546, ORPS-5037: send a custom boot_start/end PIN event for game launches/exits, with custom session header fields.
            // In order to customize the headers, this PIN event will not be queued; it will be sent immediately.
            if (!isEmbeddedBrowserAndOffline()) {
                GamesDataFactory.getCatalogInfo([offerId])
                    .then(ObjectHelperFactory.getProperty(offerId))
                    .then(_.partialRight(sendCustomBootStartEnd, isStarting));
            }

        }

        /**
         * Client download phase states to PIN download status.
         * See: https://developer.ea.com/display/AQ/River+PIN+Taxonomy
         * @param {string} phase - download phase name
         */
        function downloadPhaseToStatus(phase) {
            if (downloadPhaseToStatusMap.hasOwnProperty(phase)) {
                return downloadPhaseToStatusMap[phase];
            }
            return 'error';
        }

        /**
         * Callback function game state changes. 'playing' and 'ito' states are filtered.
         * Also used to handle download/installed state
         * @param {object} evt - event object
         */
        function onGameUpdate(evt) {
            if (!evt || !evt.signal) {
                return;
            }

            // signal look like update:OFFER_ID or progressupdate:OFFER_ID
            var signal = evt.signal;
            if (_.startsWith(signal, 'update:') || _.startsWith(signal, 'progressupdate:')) {
                var source = isEmbeddedBrowser ? 'client' : 'web';
                var offerId = signal.replace(/(update|progressupdate):/, '');
                var gameState = GamesClientFactory.getGame(offerId);
                if (!gameState) {
                    return;
                }

                // save off offerId for playing & ITO game
                if (gameState.playing || gameState.ito) {
                    telemetryLog('onGameUpdate() ' + offerId + ', playing=' + gameState.playing + ', ito=' + gameState.ito);
                    lastOfferId = offerId;
                }

                // update state change
                var prevGameState = getGameState(offerId);
                var info;
                if (!prevGameState.playing && gameState.playing) {
                    sendGameLaunch(true, offerId);
                }
                if (prevGameState.playing && !gameState.playing) {
                    sendGameLaunch(false, offerId);
                }
                if (prevGameState.installed && !gameState.installed) {
                    telemetryLog('onGameUpdate() UNINSTALLED: ' + offerId);
                    info = {
                        id: offerId,
                        state: 'UNINSTALLED'
                    };
                    Origin.telemetry.sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'download', source, offerId, info);
                }

                // download progress
                if (gameState && gameState.progressInfo) {
                    var prevProgressInfo = prevGameState.progressInfo || {};
                    // var downloadReason = lastAction.target === offerId && lastAction.action in downloadActions ? 'manual' : 'automatic';
                    var downloadReason = 'automatic';
                    var dt = Date.now() - (downloadClickedTS || 0);
                    if (dt < 15*1000) {
                        downloadReason = 'manual';
                    }
                    var phase = gameState.progressInfo.phase;
                    // If we're begining a download then set the operation type (download/update/repair)
                    if (phase === 'INITIALIZING') {
                        if (gameState.downloading) {
                            gameState.operationType = OPERATION_DOWNLOAD;
                        } else if (gameState.updating) {
                            gameState.operationType = OPERATION_UPDATE;
                        } else if (gameState.repairing) {
                            gameState.operationType = OPERATION_REPAIR;
                        }
                    } else {
                        gameState.operationType = prevGameState.operationType;
                    }

                    if (prevProgressInfo.hasOwnProperty('active') &&
                        (prevProgressInfo.active !== gameState.progressInfo.active ||
                            prevProgressInfo.phase !== gameState.progressInfo.phase ||
                            gameState.progressInfo.progress === 1)) {

                        info = {};
                        //_.merge(info, gameState.progresInfo);
                        info.id = offerId;
                        info.reason = downloadReason;

                        // See https://developer.ea.com/display/AQ/River+PIN+Taxonomy
                        /* jshint camelcase: false */
                        info.item_id = offerId;
                        info.item_type = 'offer_id';
                        info.item_platform = 'origin';
                        info.status = downloadPhaseToStatus(phase);
                        info.status_code = downloadReason;
                        info.percent = gameState.progressInfo.progress;
                        info.state = gameState.progressInfo;
                        /* jshint camelcase: true */
                        // we don't want to stringify if logging it not enabled
                        if (isTelemetryLogEnabled()) {
                            telemetryLog('onGameUpdate() DOWNLOADING:' + JSON.stringify(info, null, 2));
                        }
                        // To reduced PIN load, only send download telemetry for normal downloads - not for updates or repairs.
                        if (gameState.operationType === OPERATION_DOWNLOAD) {
                            sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'download', source, offerId, info);
                        }
                        // Send the GA events
                        switch (phase) {
                        case 'CANCELLING':
                            sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Download Queue', 'Download - Cancel', offerId);
                            break;
                        case 'PAUSING':
                            sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Download Queue', 'Download - Pause', offerId);
                            break;
                        case 'INITIALIZING':
                            sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Download Queue', 'Download - Start', offerId);
                            break;
                        case 'COMPLETED':
                            sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Download Queue', 'Download - Complete', offerId);
                            break;
                        }
                    }
                }

                var copyGameState = _.cloneDeep(gameState);
                copyGameState.sessionId = prevGameState.sessionId;
                setGameState(offerId, copyGameState);
            }
        }

        /**
         * Start the idle watch session
         */
        function startIdleWatch() {
            telemetryLog('[SESSION] IdleWatch');
            session.startTime = Date.now();
            session.endTime = 0;
            session.id = Origin.utils.generateUUID();
            Idle.watch();

            sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'session', 'start', session.id, {
                'id': session.id,
                'status': 'start',
                'stime': session.startTime
            }, session.startTime);
        }

        /**
         * Friends callback telemetry functions
         */

        /**
         * Send the friendship telemetry data
         * More info: https://confluence.ea.com/pages/viewpage.action?spaceKey=EBI&title=Origin+X+PIN+Events#OriginXPINEvents-12.friends-(engagement/social/friends)friends
         *
         * @param {number} nucleusId - nucleus id of the user to act on
         * @param {string} action - string to identify the action.
         */
        function sendFriendshipTelemetry(nucleusId, action, subSource) {
            var source = Origin.client.isEmbeddedBrowser() ? 'client' : 'web';

            sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'friends', source, 'success', {
                'frid': nucleusId,
                'friend_type': 'nucleus',
                'network': 'origin',
                'source': subSource,
                'action': action
            });
        }

        /**
         * Remove a friend
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRemove(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'remove');
        }

        /**
         * Remove & block a friend
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRemoveBlock(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'remove_block');
        }

        /**
         * Block a friend
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendBlock(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'block');
        }

        /**
         * Unblock a friend
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendUnblock(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'unblock');
        }

        /**
         * Accept friend request
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRequestAccept(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'invite_accepted');
        }

        /**
         * Reject friend request
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRequestReject(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'invite_declined');
        }

        /**
         * Send friend request
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRequestSend(nucleusId, source) {
            sendFriendshipTelemetry(nucleusId, 'invite' , source);
        }

        /**
         * Cancel friend request
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRequestCancel(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'invite_cancelled');
        }

        /**
         * Cancel and block friend request
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRequestCancelBlock(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'invite_cancelled');
        }

        /**
         * Ignore and block friend request
         * @param {number} nucleusId - nucleus id of the user to act on
         */
        function onFriendRequestIgnoreBlock(nucleusId) {
            sendFriendshipTelemetry(nucleusId, 'ignore_block');
        }

        /**
         * handle friend recommendation dismiss event.
         * @param additionalPinEventData
         */
        function onFriendRecommendationDismiss(additionalPinEventData) {
            sendFriendRecommendationTelemetryEvent(additionalPinEventData, 'Recommendation Dismiss');
        }

        /**
         * handle friend recommendation view profile event.
         * @param additionalPinEventData
         */
        function onFriendRecommendationMoreInfo(additionalPinEventData) {
          sendFriendRecommendationTelemetryEvent(additionalPinEventData, 'Recommendation More Info');
        }

        /**
         * handle friend recommendation add event.
         * @param additionalPinEventData
         */
        function onFriendRecommendationAdd(additionalPinEventData) {
          sendFriendRecommendationTelemetryEvent(additionalPinEventData, 'Recommendation Send');
        }

        /**
         *  Helper function to send out event for add/dismiss/view
         * @param additionalPinEventData
         * @param action
         */
        function sendFriendRecommendationTelemetryEvent(additionalPinEventData, action) {
          Origin.telemetry.sendTelemetryEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING,
              'Friends & Chat', 'Friend Request', action);

          sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'Friends & Chat', 'Friend Request',
              action, additionalPinEventData);
        }

        /**
         * Callback function when Angular boot is done
         */
        function bootDone() {
            startIdleWatch();
        }



        /**
         * Start listening for events that telemetry needs to track
         */
        function initEventsListener() {
            //listen for events from the JSSDK
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_EVENT, sendTelemetryEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_PIN_EVENT, sendStandardPinEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_PAGEVIEW, sendTelemetryPageView);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_TRANSACTION_EVENT, sendTransactionEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_CLIENT_ACTION, sendClientActionEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_PERFORMANCE_TIMER, sendPerformanceTimerEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_CUSTOM_DIMENSION, setCustomDimension);

            // track for spaPageLoad times
            AppCommFactory.events.on('uiRouter:stateChangeStart', onPageViewStart);
            AppCommFactory.events.on('uiRouter:stateChangeSuccess', onPageViewDone);
            AppCommFactory.events.on('uiRouter:stateChangeError', onPageViewDone);

            // track for game download time
            GamesClientFactory.events.on('GamesClientFactory:update', onGameUpdate);
            GamesClientFactory.events.on('GamesClientFactory:progressupdate', onGameUpdate);

            // Track Idle
            var interuptEvents = 'mousemove keydown DOMMouseScroll mousewheel mousedown';
            $document.find('html').on(interuptEvents, function() {
                if (!Idle.running()) {
                    startIdleWatch();
                }
            });
            $rootScope.$on('IdleWarn', function(e, countdown) {
                telemetryLog('[SESSION] IdleWarn ' + countdown);
            });

            $rootScope.$on('IdleTimeout', function() {
                session.endTime = Date.now();
                var duration = (session.endTime - session.startTime);
                telemetryLog('[SESSION] IdleTimeout id=' + session.id + ', duration=' + duration);
                sendStandardPinEvent(Origin.telemetry.trackerTypes.TRACKER_MARKETING, 'session', 'end', session.id, {
                    'id': session.id,
                    'status': 'end',
                    'stime': session.startTime,
                    'etime': session.endTime,
                    'dur': duration
                }, duration);
            });

            // Social events
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REMOVE, onFriendRemove);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REMOVE_BLOCK, onFriendRemoveBlock);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_BLOCK, onFriendBlock);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_UNBLOCK, onFriendUnblock);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_ACCEPT, onFriendRequestAccept);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_REJECT, onFriendRequestReject);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_SEND, onFriendRequestSend);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_CANCEL, onFriendRequestCancel);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_CANCEL_BLOCK, onFriendRequestCancelBlock);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_REQUEST_IGNORE_BLOCK, onFriendRequestIgnoreBlock);

            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_RECOMMENDATION_ADD, onFriendRecommendationAdd);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_RECOMMENDATION_DISMISS, onFriendRecommendationDismiss);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_FRIEND_RECOMMENDATION_MORE_INFO, onFriendRecommendationMoreInfo);
        }

        /**
         * we have to manually(not triggered from the routing system) send a pageView event to GA before sending any other events to GA,
         * to establish a landing page for the initial GA telemetry.
         */
        function setInitialGALandingPage() {
            var params = {};
            params.toid = $state.current.name;
            params.tourl = $location.url();
            params.fromState = '^';
            params.fromid = '';
            params.pgdur = 0;
            params.title = window.document.title;
            sendTelemetryPageView($location.url(), window.document.title, params);
        }

        /**
         * initializes GA, creates and initialize the trackers
         */
        function init() {
            telemetryLog('telemetryInit');

            isEmbeddedBrowser = Origin.client.isEmbeddedBrowser();
            pinTid = isEmbeddedBrowser ? ORIGINX_TID_CLIENT : ORIGINX_TID_WEB;

            //if we're embedded and offline at startup, we can't send SPA telemetry
            if (isEmbeddedBrowserAndOffline()) {
                return;
            }

            // only read the setting if in the client
            if (isEmbeddedBrowser) {
                Origin.client.settings.readSetting('TelemetryXML').then(setTelemetryXML);
            }

            //Setup common params

            setPinPlatform( Origin.utils.os());

            // Set the environment for telemetry events
            if (APPCONFIG.overrides.env === '') {
                // Production environment.  Is this a beta-opt-in version?
                if (APPCONFIG.overrides.version.search('beta') !== -1) {
                    releaseType = 'beta';
                }
            } else {
                // Non-production environment.
                releaseType = 'dev';
            }

            // Get the internal campaign id from the URL query paramaters, if there is one.  When game sites link to Origin (for example, Battlefield
            // site link to purchase BF1 through Origin), the URL includes a query parameter to indicate what site (Battlefield, in this case)
            // referred the user to Origin.
            internalCampaign = $location.search().intcmp;

            //We need an anonymous user ID that we send to pin to uniquely identify an anonymous user. Unfortunately I think this will need to be stored in a cookie
            // and will not be unique across browsers/machines so it will be essentially the GA clientId.  We can set our own clientId for GA so that PIN and
            // GA use the same ID if we care.

            //Documenation for GA Client and User IDs  https://developers.google.com/analytics/devguides/collection/analyticsjs/cookies-user-id?hl=en

            // specifying auto will create a cookie with correct domain
            //map the default one for marketing to use
            ga('create', ORIGINX_MARKETING_TRACKING_ID, 'auto', {
                'clientId': getClientId()
            });

            // create a named tracker for our dev related events
            // all our performance related events go here:
            // - only 'timing' types are currently sent in prod to not spam ga
            // - the other events are gated in sendEventToGA by the isDevTelemetryEnabled() check
            ga('create', ORIGINX_DEV_TRACKING_ID, 'auto', ORIGIN_X_SPA);

            // set reasonable default
            clientId = getClientId();
            sessionId = clientId + ':' + currentDate.getTime();

            // Setup ga ecommerce plug-in.
            ga('require', 'ecommerce');

            setInitialGALandingPage();

            // initialize page view load time tracker
            initEventsListener();
        }

        return {
            init: init,
            sendBootStartEvent: sendBootStartEvent,
            bootDone: bootDone
        };
    }

    /**
     * @function
     * @description Configure the ngIdle system
     */
    function ngIdleConfig(IdleProvider, KeepaliveProvider, TitleProvider) {
        var
            IDLE_TIMEOUT = 30*60, // session idle timeout in seconds
            IDLE_COUNTDOWN = 10; // countdown to timeout

        // configure Idle settings
        IdleProvider.idle(IDLE_TIMEOUT); // in seconds (30 minutes)
        IdleProvider.timeout(IDLE_COUNTDOWN); // in seconds
        IdleProvider.keepalive(false);
        TitleProvider.enabled(false);
    }

    /**
     * @ngdoc service
     * @name originApp.factories.TelemetryFactory
     * @description
     *
     * Telemetry Factory
     */
    angular.module('originApp')
        .config(ngIdleConfig)
        .factory('TelemetryFactory', TelemetryFactory);
}());
