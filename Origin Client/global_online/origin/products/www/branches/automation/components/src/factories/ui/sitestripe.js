/**
 * @file common/sitestripe.js
 */
(function() {
    'use strict';
    var EVENT_DELAY_MS = 100;
    function GlobalSitestripeFactory(UtilFactory, AppCommFactory, ComponentsConfigHelper, AuthFactory, TemplateResolver, AnimateFactory, $window, $state, $timeout) {
        var stripes = [],
            stripeAreas = [],
            stripeMargins = [];

        /**
        * Send a js event after a certain period of time to let
        * any margin changes propagate
        * @param {String} eventName
        */
        function sendEvent(eventName) {
            $timeout(function() {
                AppCommFactory.events.fire(eventName);
            }, EVENT_DELAY_MS, false);
        }

        function sendTelemetryEvent(){
            $timeout(function() {
                Origin.telemetry.events.fire('originGlobalSitestripeVisible');
            }, EVENT_DELAY_MS, false);

        }

        /**
         * Returns the site stripe with the specified id
         * @param {number} id - unique id to identify this stripe
         */
        function getStripe(id) {
            return _.find(stripes, function(stripe) { return stripe.id === id; });
        }

        /**
         * Adds a site stripe to be displayed based on priority (highest first). If the stripe ids already exists update the stripe.
         * @param {number} id - unique id to identify this stripe
         * @param {number} priority - priority of the stripe (displayed high to low)
         * @param {function} closeCallback - called when stripe is closed
         * @param {string} message - message text
         * @param {string} reason - reason text
         * @param {string} cta - cta text
         * @param {function} ctaCallback - called when cta is selected
         * @param {string} icon - icon to use (passive, warning, error)
         * @param {string} cta2 - cta2 text
         * @param {function} cta2Callback - called when cta2 is selected
         * @param {boolean} permanent - if true the stripe cannot be closed
         * @param {boolean} winFilter - if true do not show on windows
         * @param {boolean} macFilter - if true do not show on mac
         * @param {boolean} webFilter - if true do not show on web
         * @param {number} greaterVersionFilter - if version is greater or equal do not show
         * @param {number} lesserVersionFilter - if version is lesser or equal do not show
         * @param {boolean} betaFilter - if version is beta do not show
         * @param {array} showOnStates - show if current url is in list
         * @param {array} hideOnStates - hide if current url is in list
         */
        function showStripe(id, priority, closeCallback, message, reason, cta, ctaCallback, icon, cta2, cta2Callback, permanent, winFilter, macFilter, webFilter, greaterVersionFilter, lesserVersionFilter, betaFilter, showOnStates, hideOnStates, getCtaUrl, getCta2Url) {
            if (ComponentsConfigHelper.getHideSiteStripeConfig()) {
                return;
            }

            var stripe = getStripe(id);
            if (stripe) {
                updateStripe(stripe, message, reason, ctaCallback);
            } else {
                stripe = {
                    id: id,
                    priority: priority,
                    message: message,
                    reason: reason,
                    cta: cta,
                    cta2: cta2,
                    closeCallback: closeCallback,
                    ctaCallback: ctaCallback,
                    getCtaUrl: getCtaUrl,
                    cta2Callback: cta2Callback,
                    getCta2Url: getCta2Url,
                    icon: icon,
                    permanent: permanent,
                    winFilter: winFilter,
                    macFilter: macFilter,
                    webFilter: webFilter,
                    greaterVersionFilter: greaterVersionFilter,
                    lesserVersionFilter: lesserVersionFilter,
                    betaFilter: betaFilter,
                    showOnStates: showOnStates,
                    hideOnStates: hideOnStates,
                    filtered: true
                };

                normalizeStripe(stripe);
                addStripe(stripe);
            }
        }

        /**
         * Removes the site stripe with the specified id
         * @param {number} id - unique id to identify this stripe
         */
        function hideStripe(id) {
            if (ComponentsConfigHelper.getHideSiteStripeConfig()) {
                return;
            }

            removeStripe(id);
        }

        /**
         * Update a site stripe, if active stripe also update areas and margins
         * @param {object} stripe - the stripe to update
         * @param {string} message - message text
         * @param {string} reason - reason text
         * @param {function} ctaCallback - udpate the callback function
         */
        function updateStripe(stripe, message, reason, ctaCallback) {
            if (stripe) {
                if (message) {
                    stripe.message = message;
                }

                if (reason) {
                    stripe.reason = reason;
                }

                if (ctaCallback && ctaCallback !== stripe.ctaCallback) {
                    stripe.ctaCallback = ctaCallback;
                }

                if (stripe === getActiveStripe()) {
                    updateStripeAreas();
                    updateMargins();
                    sendEvent('globalsitestripe:update');
                    sendTelemetryEvent();
                }
            }
        }

        /**
         * Sets stripe.filtered to true or false based on conditions
         * @param {object} stripe - stripe to run filter logic
         */
        function filterStripe(stripe) {
            stripe.filtered = false;

            var url = $state.href($state.current.name, $state.params);
            if (stripe.showOnStates.length) {
                if (_.findIndex(stripe.showOnStates, function(item) { return _.includes(url, item); }) === -1) {
                    stripe.filtered = true;
                    return;
                }
            }

            if (stripe.hideOnStates.length) {
                if (_.findIndex(stripe.hideOnStates, function(item) { return _.includes(url, item); }) !== -1) {
                    stripe.filtered = true;
                    return;
                }
            }

            if (Origin.client.isEmbeddedBrowser()) {
                if (stripe.winFilter && Origin.utils.os() === Origin.utils.OS_WINDOWS) {
                    stripe.filtered = true;
                    return;
                }

                if (stripe.macFilter && Origin.utils.os() === Origin.utils.OS_MAC) {
                    stripe.filtered = true;
                    return;
                }

                var version = Origin.client.info.versionNumber();
                if ((stripe.greaterVersionFilter !== false && version >= stripe.greaterVersionFilter) ||
                    (stripe.lesserVersionFilter !== false && version <= stripe.lesserVersionFilter)) {
                    stripe.filtered = true;
                    return;
                }

                if (stripe.betaFilter && Origin.client.info.isBeta()) {
                    stripe.filtered = true;
                    return;
                }
            } else {
                if (stripe.webFilter) {
                    stripe.filtered = true;
                    return;
                }
            }
        }

        /**
         * Defaults members that are required for GlobalSitestripeFactory to work correct
         * @param {object} stripe - stripe to initialize
         */
        function normalizeStripe(stripe) {
            if (_.isUndefined(stripe.priority)) {
                stripe.priority = 0;
            }

            if (_.isUndefined(stripe.winFilter)) {
                stripe.winFilter = false;
            }

            if (_.isUndefined(stripe.macFilter)) {
                stripe.macFilter = false;
            }

            if (_.isUndefined(stripe.webFilter)) {
                stripe.webFilter = false;
            }

            if (_.isUndefined(stripe.greaterVersionFilter)) {
                stripe.greaterVersionFilter = false;
            }

            if (_.isUndefined(stripe.lesserVersionFilter)) {
                stripe.lesserVersionFilter = false;
            }

            if (_.isUndefined(stripe.betaFilter)) {
                stripe.betaFilter = false;
            }

            if (_.isUndefined(stripe.showOnStates)) {
                stripe.showOnStates = "";
            }

            if (_.isUndefined(stripe.hideOnStates)) {
                stripe.hideOnStates = "";
            }

            filterStripe(stripe);
        }

        /**
         * Returns the last stripe which is not filter
         * @return {object} the active stripe
         */
        function getActiveStripe() {
            var index = _.findLastIndex(stripes, function(stripe) { return !stripe.filtered; });
            return index !== -1 ? stripes[index] : undefined;
        }

        /**
         * Adds a stripe to the factory for display
         * @param {object} stripe to add
         */
        function addStripe(stripe) {
            // push stripe onto array and sort to ensure priority is correct
            stripes.push(stripe);
            stripes = _.sortBy(stripes, function(item) { return item.priority; });

            // update the areas and margins incase a different stripe is now active
            updateStripeAreas();
            updateMargins();
            sendEvent('globalsitestripe:add');
            sendTelemetryEvent();
        }

        /**
         * Removes the site stripe with the specified id
         * @param {number} id - unique id to identify this stripe
         */
        function removeStripe(id) {
            _.remove(stripes, function(stripe) { return stripe.id === id; });

            // update the areas and margins incase a different stripe is now active
            updateStripeAreas();
            updateMargins();
            sendEvent('globalsitestripe:remove');
        }


        /**
         * Add an area to recieve stripe data
         * @param {function} setDataCallback - called when there is new site stripe data to show
         * @param {function} getHeightCallback - called when this is the active site stripe to set margins
         */
        function addStripeArea(setDataCallback, getHeightCallback) {
            var stripeArea = {
                setDataCallback: setDataCallback,
                getHeightCallback: getHeightCallback
            };

            stripeAreas.push(stripeArea);
            updateStripeArea(stripeArea);

            // if this is the first area we are adding we can use it to update margins
            if (stripeAreas.length === 1) {
                updateMargins();
            }
        }

        /**
         * Send the active stripe data to the area
         * @param {object} stripeArea - the area to recieve the active stripe data
         */
        function updateStripeArea(stripeArea) {
            stripeArea.setDataCallback(getActiveStripe());
        }

        /**
         * Send the active stripe data to all areas
         */
        function updateStripeAreas() {
            _.forEach(stripeAreas, updateStripeArea);
        }


        /**
         * Add a margin to recieve stripe heights
         * @param {function} setMarginCallback - called when there is new site stripe to update height or margin
         * @return {function} call to remove the margin
         */
        function addStripeMargin(setMarginCallback) {
            var stripeMargin = {
                setMarginCallback: setMarginCallback
            };

            stripeMargins.push(stripeMargin);
            updateMargin(stripeMargin);

            // return a handle to destroy
            return function() {
                _.remove(stripeMargins, function(item) { return item === stripeMargin; });
            };
        }

        /**
         * Send the active stripe height to the margin
         * @param {object} stripeMargin - the margin to recieve the active stripe height
         */
        function updateMargin(stripeMargin) {
            // HACK for now we only support one area to update all margins.
            if (stripeAreas.length) {
                stripeAreas[0].getHeightCallback(stripeMargin.setMarginCallback);
            } else {
                stripeMargin.setMarginCallback(0);
            }
        }

        /**
         * Send the active stripe height to all margins
         */
        function updateMargins() {
            _.forEach(stripeMargins, updateMargin);
        }


        /**
         * Update filters for all stripes then updates areas and margins
         */
        function updateFilters() {
            _.forEach(stripes, filterStripe);
            updateStripeAreas();
            updateMargins();
        }

        function retrieveTemplate() {
            return TemplateResolver.getTemplate('global-sitestripe', {}, true);
        }
        //RS: Todo - these events are not getting cleaned up properly
        // This will be addressed in another branch 
        AnimateFactory.addResizeEventHandler(undefined, updateMargins, 300);
        AppCommFactory.events.on('uiRouter:stateChangeSuccess', updateFilters);

        return {
            showStripe: showStripe,
            hideStripe: hideStripe,
            addStripeArea: addStripeArea,
            addStripeMargin: addStripeMargin,
            retrieveTemplate: retrieveTemplate,
            updateFilters: updateFilters
        };
    }


    /**
     * Calls singleton factory to get the instance of this function stored with the main SPA (even when called from a popup)
     * @param {object} SingletonRegistryFactory the factory which manages all the other factories
     */
    function GlobalSitestripeFactorySingleton(UtilFactory, AppCommFactory, ComponentsConfigHelper, AuthFactory, TemplateResolver, AnimateFactory, $window, $state, $timeout, SingletonRegistryFactory) {
        return SingletonRegistryFactory.get('GlobalSitestripeFactory', GlobalSitestripeFactory, this, arguments);
    }

    /**
     * @ngdoc service
     * @name origin-components.factories.GlobalSitestripeFactory

     * @description
     *
     * GlobalSitestripeFactory
     */
    angular.module('origin-components')
        .factory('GlobalSitestripeFactory', GlobalSitestripeFactorySingleton);
}());
