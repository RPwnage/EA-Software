/**
* @file origincomponents.js
*/
(function() {

    function bubbleSetup($rootScope, AuthFactory, ComponentsLogFactory) {

        var proto = Object.getPrototypeOf($rootScope);

        /**
         * Set the count of the number of children if we
         * have not calculated it already
         * @return {void}
         * @method _setChildCount
         */
        proto._setChildCount = function() {
            if (!this.hasOwnProperty('childCount')) {
                this.childCount = 0;
                if (this.$$childTail) {
                    this.childCount = (this.$$childTail.$index || 0) + 1;
                }
            }
        };

        /**
         * Set the number of children that are ready to 0
         * if it has not been set already
         * @method _initReadyChildren
         * @return {void}
         */
        proto._initReadyChildren = function() {
            if (!this.hasOwnProperty('readyChildren')) {
                this.readyChildren = 0;
            }
        };

        /**
         * Bubble the ready event event upwards if the element is ready
         * and the children are all ready as well
         * @method _bubbleReady
         * @return {void}
         */
        proto._bubbleReady = function() {
            if ((this.isReady || !this.$parent) && (this.readyChildren === this.childCount)) {
                if (this.$parent) {
                    this.$parent._childReady();
                } else {
                    this.isDone = true;
                    ComponentsLogFactory.log('Victory!');
                }
            }
        };

        /**
         * A child is ready, update the number of children ready
         * and if all of the children are ready then bubble the
         * event upwards to your parent.
         * @return {void}
         */
        proto._childReady = function() {
            this._setChildCount();
            this._initReadyChildren();
            this.readyChildren++;
            this._bubbleReady();
        };

        /**
         * When we are ready, let our parent know that we are
         * ready, and to bubble the event upwards
         * @return {void}
         */
        proto.ready = function() {
            this._setChildCount();
            this._initReadyChildren();
            this.isReady = true;
            this._bubbleReady();
        };

    }

    function initSocialFactories(ConversationFactory, RosterDataFactory, SocialDataFactory, VoiceFactory, AuthFactory, ComponentsLogFactory) {
        function init() {
            ConversationFactory.init();
            RosterDataFactory.init();
            SocialDataFactory.init();
            VoiceFactory.init();
        }

        /*
         * catch error from waitForAuthReady
         * @return {void}
         * @method
         */
        function authReadyError(error) {
            ComponentsLogFactory.error('INITSOCIALFACTORIES: authReady error', error);
        }

        AuthFactory.waitForAuthReady()
            .then(init)
            .catch(authReadyError);
    }

    function initEndpoints(ComponentsConfigHelper) {
        ComponentsConfigHelper.init();
    }

    function initUTagDataLayer(UTagDataModelFactory){
        UTagDataModelFactory.init();
    }

    function initDefaultSeoTitle(OriginSeoFactory){
        OriginSeoFactory.setSeoData('title');
    }

    function initLocFactory(LocFactory, COMPONENTSCONFIG) {
        var envString = _.get(COMPONENTSCONFIG, ['overrides', 'env'], ''),
            cmsStageString = _.get(COMPONENTSCONFIG, ['overrides', 'cmsstage'], ''),
            showMissingStringsFallback = _.get(COMPONENTSCONFIG, ['overrides', 'showmissingstringsfallback'], false),
            displayFallback = (envString.length || cmsStageString.length) && showMissingStringsFallback;

        //only show fallback strings on non prod live environments
        LocFactory.init(displayFallback);
    }

    function setupUrls(COMPONENTSCONFIG) {
        if(window.OriginKernel) {
            _.extend(COMPONENTSCONFIG, window.OriginKernel.COMPONENTSCONFIG);
        }
    }

    function startUrlTriggers(UrlTriggerFactory) {
        UrlTriggerFactory.startActions();
    }

    /**
     * @ngdoc object
     * @name origin-components
     * @description
     * @requires  origin-i18n
     * component package
     *
     *
     */
    angular.module('origin-components', ['origin-i18n', 'angular-toArrayFilter', 'origin-components-config', 'ui.router', 'ngCookies', 'eax-experiments'])
        .config(setupUrls)
        .run(bubbleSetup)
        .run(initEndpoints)
        .run(initLocFactory)
        .run(initSocialFactories)
        .run(initUTagDataLayer)
        .run(initDefaultSeoTitle)
        .run(startUrlTriggers);

}());
