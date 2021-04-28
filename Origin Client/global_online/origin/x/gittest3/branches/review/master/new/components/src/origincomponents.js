/**
* @file origincomponents.js
*/
(function() {

    function bubbleSetup($rootScope, ComponentsLogFactory) {

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

	function initSearchProviders(SearchProviderInit) {
		SearchProviderInit.init();
	}

    function initSocialFactories(ConversationFactory, RosterDataFactory, SocialDataFactory, UserDataFactory, AppCommFactory) {
        function init() {
            ConversationFactory.init();
            RosterDataFactory.init();
            SocialDataFactory.init();
        }

        //set up the connections and initiate things needed for social
        //console.log('SocialData: UserDataFactory.isAuthReady: ' + UserDataFactory.isAuthReady());
        if (UserDataFactory.isAuthReady()) {
            init();
        } else {
            //We use a events.once here, because if a user is not logged in the 'auth:ready' event gets fired twice.
            //It's fired when the initial auth state is determine AND when we log in successfully
            AppCommFactory.events.once('auth:ready', init);
        }
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
    angular.module('origin-components', ['origin-i18n', 'angular-toArrayFilter', 'origin-components-config'])
        .run(bubbleSetup)
		.run(initSearchProviders)
        .run(initSocialFactories);

}());