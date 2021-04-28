/* TelemetryFactory
 *
 * manages sending telemetry for the app
 * @file factories/telemetry.js
 */
 /* globals ga */

(function() {
    'use strict';

    //doesn't like _ in ts_event & install_type, etc.
    /*jshint camelcase: false */
    function TelemetryFactory($http, LogFactory) {

        var DEFAULT_NUCLEUS_ID = '0',
            DEFAULT_APP_VERSION = '10.0.1.0001',
            ORIGINX_MARKETING_TRACKING_ID = 'UA-49146401-5',
            ORIGINX_DEV_TRACKING_ID = 'UA-49146401-3',
            ORIGINX_TID = 'origin-2015-web',
            ORIGINX_TID_DEV = 'origindev-2015-web',
            ORIGINX_PIDT = 'origin_web',
            ORIGINX_PIDT_DEV = 'origin_web_dev',
            TRACKER_TYPE_MARKETING = 'TRACKER_MARKETING',
            TRACKER_TYPE_DEV = 'TRACKER_DEV',
            TRACKER_NAME_MARKETING = '',
            TRACKER_NAME_DEV = 'originXspa',

            step = 0,  //Telemetry Step counter
                //Need a way to create and store a sessionID
            sessionId = '0',  //GA automatically tracks a session ID in a cookie. You can tune the rules about what a session is.
                              // Usually all interactions with less than a 30 min break.
            clientId = '',
            appVersion = DEFAULT_APP_VERSION,
            tracker,
            currentDate = new Date(),
            isEmbeddedBrowser = false;

        function isEmbeddedBrowserAndOffline() {
            if (isEmbeddedBrowser && !Origin.client.onlineStatus.isOnline()) {
                return true;
            }
            return false;
        }

        function httpRequest(endPoint, config) {
            var HTTP_REQUEST_TIMEOUT = 60000,   //recommended timeout from PIN docs
                errResponse,
                data;

            if (endPoint === '') {
                errResponse.status = -1;
                return Promise.reject(errResponse);
            } else {
                config.timeout = HTTP_REQUEST_TIMEOUT;
                //reuse the config object, extract out data to post, and then clear it from the config so we're not doubly defining it
                data = config.data;
                config.data = {};

                return $http.post(endPoint, data, config);
            }
        }

        function handlePINerror(error) {
            //TODO: need to set up a retry if 500 or 503
            //DO NOT use LogFactory.error here; otherwise we'll get recursive calls to telemetr to log the error
            if (error.status === 500 || error.status === 503) {
                LogFactory.log('httpStatus = ', error.status, ', need to retry');
            } else {
                LogFactory.log('unexpected error from PIN:', error);
            }
        }

        function getPlatform(os) {
            //allowed values
            //xbox_one
            //ps4
            //xbox_360
            //ps3
            //pc
            //iOS
            //win
            //android
            //other

            if (os === 'PCWIN') {
                return 'pc';
            } else if (os === 'Mac') {
                return ('iOS');
            } else {
                return ('other');
            }
        }

        function sendPINrequest(endPoint, config) {
            //don't send request if client && offline
            if (!isEmbeddedBrowserAndOffline()) {
                httpRequest(endPoint, config)
                    .catch(handlePINerror);
            }
            //eventually may want to queue this request until we're back online
        }

        /**
         * send telemetry to PIN
         *
         * @param {string} trackerType distinguish between marketing and dev
         * @param {object} params additional data to send to PIN
         */
        function sendToPin(trackerType, params) {
            //TODO: Need to grab locale to pass as 'loc' but can't get it from JSSDK since that requires initialization
            var ts = new Date().toISOString(),
                endPoint = 'https://pin-river-test.data.ea.com/pinEvents',
                platform = getPlatform (Origin.utils.os()),
                config = {
                    headers: {
                        'Content-Type': 'application/json',
                        'x-ea-game-id':'OriginSPA',
                        'x-ea-game-id-type': 'client',
                        'x-ea-app-type': 'WEBTRACKING',
                        'x-ea-taxv': '1.1'
                    },
                    data: {
                        'taxv': '1.1',
                        'tid': (trackerType === TRACKER_TYPE_MARKETING) ? ORIGINX_TID : ORIGINX_TID_DEV,
                        'tidt': 'sku',
                        'rel': 'beta',
                        'v': '10.0.0.1',  //Origin Version
                        'ts_post': ts,
                        'sid': sessionId, //Session ID we need to make one of these
                        'plat': platform, //'pc',   //We should try to figure out the correct platform (pc, mac, ios etc.)
                        'et': 'client',
                        //'clientIP': '10.10.28.1', //Probably shouldn't send this.
                        'loc': Origin.locale.locale(),
                        'events': []
                    }
                },
                eventObj = {};

            params.pidt = (trackerType === TRACKER_TYPE_DEV) ? ORIGINX_PIDT_DEV : ORIGINX_PIDT;

            //advance step
            step++;

            //add more stuff to params
            params.ts_event = ts;
            params.sid = sessionId;
            params.s = step;

            params.pid = clientId;   //use the clientID as pid for both anonymous and logged in and use pidm to specify nucleusId if logged in

            //if user is logged in we need to get their nucleus Id
            //we're purposely not using AuthFactory in case this code needs to be called from the bootstrapper
            if (Origin.user && Origin.auth.isLoggedIn()) {
                params.pidm = {'nucleus': Origin.user.userPid()};
            } else {
                params.pidm = {'nucleus': DEFAULT_NUCLEUS_ID};
            }

            eventObj.core = params;
            eventObj.install_type = 'other';

            config.data.events.push(eventObj);

            sendPINrequest(endPoint,config);
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
                'status': 'dimension1',
                'type' : 'dimension2',
                'status_code' : 'dimension3'
            };

            var outParams = {};

            for ( var key in params ){
                if( key in gaParamFilter)
                {
                    outParams[gaParamFilter[key]] = params[key];
                }
            }

            return outParams;
        }

        function sendEventToGA(tracker, category, action, label, step, gaParams) {
            if (!isEmbeddedBrowserAndOffline()) {
                ga(tracker+'send', 'event', category, action, label, step, gaParams);
            }
            //eventually may want to queue this request until we're back online
        }

        /*
         * this function is triggered from the JSSDK when it receives a telemetry request
         *
         * @param {string} a list of arguments
         */
        function sendTelemetryEvent(eventargs) {
            //eventargs will be an array of [category, action, label, params] where params is an object
            var category,
                action,
                label,
                params,
                gaParams,
                tracker,
                trackerType;

            if (eventargs) {
                trackerType = eventargs[0];

                category = eventargs[1];
                action = eventargs[2];
                label = eventargs[3];
                params = eventargs[4];
                gaParams = packParamsAsDimension(params);

                if (trackerType === TRACKER_TYPE_MARKETING) {
                    tracker = TRACKER_NAME_MARKETING;
                } else {
                    tracker = TRACKER_NAME_DEV + '.';
/*
                    LogFactory.log( 'GA event sent category:', category, ' action:', action, ' label:', label);
                    LogFactory.log( 'Parameters are:');
                    LogFactory.log( gaParams );
*/
                }

                sendEventToGA(tracker, category, action, label, step, gaParams);

                if (typeof params === 'undefined') {
                    params = {};
                }
                params.en = category;

                sendToPin(trackerType, params);
            }
        }

        function sendCommandToGA(cmd, param) {
            if (!isEmbeddedBrowserAndOffline()) {
                ga(cmd, param);
            }
            //eventually may want to queue this request until we're back online
        }

        /**
         * sends a page view event
         *
         * @param {string} page the ui-route uri
         * @param {string}  title ui-route name
         * @param {object} params extra data
         */
        function sendTelemetryPageView(page, title, params) {
            //assume marketing
            var pageObj = {};

            pageObj.page = page;
            pageObj.title = title;

            //set it so all subsequent events will be from this "page"
            sendCommandToGA('set', pageObj);
            sendCommandToGA('send', 'pageView');

            if (typeof params === 'undefined') {
                params = {};
            }
            params.en = 'page_view';
            params.type = 'web page';
            params.pgid = page;

            sendToPin(TRACKER_TYPE_MARKETING, params);
        }

        /**
         * initializes GA, creates and initialize the trackers
         */

        function init(appVersion) {
            LogFactory.log('telemetryInit');

            isEmbeddedBrowser = Origin.client.isEmbeddedBrowser();

            //if we're embedded and offline at startup, we can't send SPA telemetry
            if (isEmbeddedBrowserAndOffline()) {
                return;
            }

           //Setup common params

            //We need an anonymous user ID that we send to pin to uniquely identify an anonymous user. Unfortunately I think this will need to be stored in a cookie
            // and will not be unique across browsers/machines so it will be essentially the GA clientId.  We can set our own clientId for GA so that PIN and
            // GA use the same ID if we care.

            //Documenation for GA Client and User IDs  https://developers.google.com/analytics/devguides/collection/analyticsjs/cookies-user-id?hl=en

//            window.ga_debug = {trace: true};

            // specifying auto will create a cookie with correct domain
            //map the default one for marketing to use
            ga('create', ORIGINX_MARKETING_TRACKING_ID, 'auto');

            //create a named tracker for our dev related events
            ga('create', ORIGINX_DEV_TRACKING_ID, 'auto', TRACKER_NAME_DEV);

            if (appVersion) {
                setAppVersion(appVersion);
            }

            //TODO: need to grab locale, but not from JSSDK since that requires initialization
            ga(function() {
                //get the named tracker so we can get the client id
                tracker = ga.getByName(TRACKER_NAME_DEV);
                if (tracker) {
                    clientId = tracker.get('clientId'); //clientId is saved in the cookie, so it would carry across browser sessions
                }

                //make the sessionId a composite of clientId and timestamp
                sessionId = clientId + ':' + currentDate.toUTCString();

                //Set the session level parameters
                ga(TRACKER_NAME_DEV+'.set', {
                  'appversion': getAppVersion(), //parameter needs tobe lower case even tho docs say otherwise (debug console will complain too),
                                                 //BUT if it isn't, it won't post any event that follows
                  'language' : Origin.locale.locale(),
                  // These should really only be set on login.
                  'metric1' : 0    //isSubscriber  - The problem with metrics is they are only valid on the hit and product level in GA. We'd need to use
                                    // a dimension to make this user level. See the Custom Definitions in GA.
                  // 'metric2' : 0      //isUnderage  This is probably PII so we shouldn't send it.
                });
            });

            //listen for events from the JSSDK
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_EVENT, sendTelemetryEvent);
            Origin.telemetry.events.on(Origin.telemetry.events.TELEMETRY_PAGEVIEW, sendTelemetryPageView);
        }

        function setAppVersion (version) {
            appVersion = version;
        }

        function getAppVersion() {
            return appVersion;
        }

        return {
            init: init,

            setAppVersion: setAppVersion
        };
        /*jshint camelcase:true*/
    }

    /**
     * @ngdoc service
     * @name originApp.factories.TelemetryFactory
     * @description
     *
     * Telemetry Factory
     */
    angular.module('originApp')
        .factory('TelemetryFactory', TelemetryFactory);
}());