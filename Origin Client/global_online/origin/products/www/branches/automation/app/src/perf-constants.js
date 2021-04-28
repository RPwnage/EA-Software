/**
 * @file perf-constants.js
 */

(function() {
    'use strict';

    window.OriginPerfConstant = {
        markers: {
            NAVIGATION_START: 'navigationStart', //this is a built in navigation timing
            HEAD_PARSED: 'HeadParsed',
            ANGULAR_BOOTSTRAP_START: 'AngularBootstrapStart',
            ROUTE_CHANGE_START: 'RouteChangeStart',
            PAGE_LOAD_COMPLETED: 'PageLoadCompleted'
        },
        measure: {
            START_TO_HEAD: 'StartToHeadFinished',
            HEAD_TO_ANGULAR_START: 'HeadFinishedToAngularStart',
            ANGULAR_START_TO_FIRST_ROUTE: 'AngularStartToFirstRoute',
            FIRST_ROUTE_START_TO_PAGE_LOAD: 'FirstRouteStartToPageLoad',
            ROUTE_START_TO_PAGE_LOAD: 'RouteStartToPageLoad',
            START_TO_FIRST_PAGE_LOAD: 'StartToFirstPageLoad',
            START_TO_ANGULAR_START: 'StartToAngularStart'
        }
    };
}());