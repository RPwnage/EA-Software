/**
 * The routing ng module
 */
(function () {
    'use strict';

    var lastPageLoadTimestamp = new Date().valueOf();

    function getDurationOfStay() {
        var now = new Date().valueOf();
        var duration = Math.round((now - lastPageLoadTimestamp) / 1000);
        lastPageLoadTimestamp = now;
        return duration;
    }

    function sendRouteChangeTelemetry(toState, fromState, url) {
        var params = {};
        params.toid = toState.name;
        params.tourl = url;
        params.fromState = fromState.url;
        params.fromid = fromState.name;
        params.pgdur = getDurationOfStay();
        params.title = window.document.title;
        Origin.telemetry.sendPageView(params.tourl, window.document.title, params);
    }

    /**
     * Get the name of the query parameters in the order
     * that they appear in the query string
     * @return {Array} query parameter names
     */
    function getQueryParamNames() {
        var search = window.location.search,
            queryParams = [];
        if (search) {
            search.substring(1).split('&').forEach(function (item) {
                queryParams.push(item.split('=')[0]);
            });
        }
        return queryParams;
    }

    /**
     * Order the pass through query params in the order that they
     * appear on the initial page load as query parameters
     * @param {Array} defaultParams - the default list of query params to be ordered
     * @return {Array}
     */
    function orderPassThroughQueryParams(defaultParams) {
        var queryParams = getQueryParamNames(),
            orderedParams = [];
        for (var i = 0, j = queryParams.length; i < j; i++) {
            var paramIndex = defaultParams.indexOf(queryParams[i]);
            if (paramIndex !== -1) {
                orderedParams.push(defaultParams[paramIndex]);
                defaultParams.splice(paramIndex, 1);
            }
        }
        return orderedParams.concat(defaultParams);
    }

    /**
     * Pass along query parameters in the URL so that we can easily bookmark them
     * @param {Array} OriginPassThroughQueryParams - array of params to pass through
     * @param {Object} fromParams
     * @param {Object} toParams
     **/
    function passAlongQueryParameters(OriginPassThroughQueryParams, fromParams, toParams) {
        _.forEach(OriginPassThroughQueryParams, function (param) {
            if (!!fromParams[param] && !toParams[param]) {
                toParams[param] = fromParams[param];
            }
        });
        return toParams;
    }

    /**
     * Initialize the origin-components compatibility module.
     * @param  {Object} $rootScope     instance of $rootScope from angular
     * @param  {Object} $state         instance of $state from ui.router
     * @param  {Object} $location      instance of $location from angular
     * @param  {Object} AppCommFactory instance of AppCommFactory from origin-components
     * @param  {Array}  OriginPassThroughQueryParams - array of params to pass through
     */
    function initEventHandlers($rootScope, $state, $location, AppCommFactory, OriginPassThroughQueryParams) {

        $rootScope.$on('$stateChangeStart', function (event, toState, toParams, fromState, fromParams) {
            toParams = passAlongQueryParameters(OriginPassThroughQueryParams, fromParams, toParams);
            AppCommFactory.events.fire('uiRouter:stateChangeStart', event, toState, toParams, fromState, fromParams);
        });

        $rootScope.$on('$stateChangeSuccess', function (event, toState, toParams, fromState, fromParams) {
            AppCommFactory.events.fire('uiRouter:stateChangeSuccess', event, toState, toParams, fromState, fromParams);
            sendRouteChangeTelemetry(toState, fromState, $location.url());
        });

        $rootScope.$on('$stateChangeError', function (event, toState, toParams, fromState, fromParams, error) {
            AppCommFactory.events.fire('uiRouter:stateChangeError', event, toState, toParams, fromState, fromParams, error);
        });

        AppCommFactory.events.on('uiRouter:go', function (toName, toParams, toOptions) {
            $state.go(toName, toParams, toOptions);
        });
    }

    /**
     * Get the configuration for origin-routing
     * @param {Object} $urlRouterProvider an instance of $urlRouerProvider from ui.router
     */
    function setupRewrites($urlRouterProvider) {
        $urlRouterProvider
            .when('', function($injector, $location) {
                $injector.invoke(['$state', function($state) {
                    $state.go('app.home_home', $location.search(), {location : 'replace'});
                }]);
            })
            .when('/', function($injector, $location) {
                $injector.invoke(['$state', 'AuthFactory', function($state, AuthFactory) {
                    if(AuthFactory.isAppLoggedIn() ) {
                        $state.go('app.home_loggedin', $location.search(), {location : 'replace'});
                    } else {
                        $state.go('app.home_home', $location.search(), {location : 'replace'});
                    }
                }]);
            })
            .when('/store', function($injector, $location) {
                $injector.invoke(['$state', function($state) {
                    $state.go('app.store.wrapper.home', $location.search(), {location : 'replace'});
                }]);
            }) // remove this after you merge this in and fix up the store routes
            .when('/game-library/ogd/:offerid', function($match, $injector, $location) {
                $injector.invoke(['$state', function($state) {
                    var obj = _.merge($match, $location.search(), {
                        'topic': 'friends-who-play'
                    });
                    $state.go('app.game_gamelibrary.ogd.topic', obj, {location : 'replace'});
                }]);
            })
            .when('/profile', function($injector, $location) {
                $injector.invoke(['$state', function($state) {
                    var obj = _.merge($location.search(), {
                        'topic': 'achievements'
                    });
                    $state.go('app.profile.topic', obj, {location : 'replace'});
                }]);
            })
            .when('/discover', function($injector, $location) {  //remove this after we've updated all the CQ from discover to my-home, but need this so we don't break
                $injector.invoke(['$state', function($state) {
                    $state.go('app.home_home', $location.search(), {location : 'replace'});
                }]);
            })
            .when('/profile/user/:id', function($match, $injector, $location) {
                $injector.invoke(['$state', function($state) {
                    var obj = _.merge($match, $location.search(), {
                        'topic': 'achievements'
                    });
                    $state.go('app.profile.user.topic', obj, {location : 'replace'});
                }]);
            })
            .otherwise(function($injector, $location) {
                $injector.invoke(['$state', function($state) {
                    $state.go('app.error_notfound', $location.search(), {location : 'replace'});
                }]);
            });
    }

    /**
     * reload the current route
     */
    function reloadCurrentRouteOnAuthChange(RoutingHelperFactory, AuthFactory) {
        return function () {
            //If we're logging out the page will refresh instead
            if (AuthFactory.isAppLoggedIn()) {
                RoutingHelperFactory.reloadCurrentRoute();
            }
        };
    }

    /**
     * setup a listener for auth change to reload the content view pannel
     * @param  {Object} AuthFactory instance of AuthFactory from components
     * @param {Object} RoutingHelperFactory
     */
    function listenForAuthChangeForRouteReload(AuthFactory, RoutingHelperFactory) {
        AuthFactory.events.on('myauth:change', reloadCurrentRouteOnAuthChange(RoutingHelperFactory, AuthFactory));
    }

    /**
     * Injects TemplateResolver at runtime and loads a template from CQ5 based on given key.
     * Look at TemplateResolver factory for more info.
     * @param templateKey
     * @returns {Array}
     */
    function resolveTemplate(templateKey) {
        //Note we must inject dependencies manually. ngAnnotate can't do this
        return ['TemplateResolver', '$stateParams', function (TemplateResolver, $stateParams) {
            return TemplateResolver.getTemplate(templateKey, $stateParams);
        }];
    }

    /**
     * Checks the incoming OGD offer and redirects to owned basegame for DLC's and addons
     * @returns {Promise}
     */
    function checkForOgdRedirect() {
        return ['RoutingHelperFactory', '$stateParams', function (RoutingHelperFactory, $stateParams) {
            return RoutingHelperFactory.checkForOgdRedirect(_.get($stateParams, 'offerid'));
        }];
    }

    /**
     * Before rendering the view, wait for supercat to download all offers.
     * @returns {Array}
     */
    function waitForGamesData() {
        //Note we must inject dependencies manually. ngAnnotate can't do this
        return ['GamesDataFactory', function (GamesDataFactory) {
            return GamesDataFactory.waitForGamesDataReady();
        }];
    }

    /** 
     * wait for dictionary to load
     */
    function waitForDictionary() {
        return ['LocDictionaryFactory', 'APPCONFIG',
            function(LocDictionaryFactory, APPCONFIG) {

                if (LocDictionaryFactory.isLoaded()) {
                    return Promise.resolve();
                } else {
                    return OriginKernel.promises.getDefaults
                        .then(function(data) {
                            var usefixture = APPCONFIG.overrides.usefixturefile || true;
                            LocDictionaryFactory.init(data, usefixture, APPCONFIG.overrides.showmissingstrings, APPCONFIG.overrides.showcomponentnames);
                            //clear the promise to release any hold on the data
                            OriginKernel.promises.getDefaults = null;
                        });

                }
            }
        ];
    }
    /**
     * Before rendering the view, wait for auth ready
     * @returns {Array}
     */
    function waitForAuthReady() {
        //Note we must inject dependencies manually. ngAnnotate can't do this
        return ['AuthFactory', function (AuthFactory) {
            return AuthFactory.waitForAuthReady();
        }];
    }

    /**
     * Bro. Relax your url requirements.
     * @param {object} $urlMatcherFactoryProvider
     */
    function disableStrictMode($urlMatcherFactoryProvider) {
        $urlMatcherFactoryProvider.strictMode(false);
    }

    /**
     * Before rendering any subscriptions view, check if the current country is disabled,
     * and redirect to coming soon page if it is disabled, or redirect away from the coming
     * soon page if it is enabled
     *
     * @param {Boolean} comingSoon - flag to determine if this is a coming soon page request
     * @returns {Promise}
     */
    function isCountrySubscriptionEnabled(comingSoon) {
        //Note we must inject dependencies manually. ngAnnotate can't do this
        return ['APPCONFIG', '$state', function (APPCONFIG, $state) {

            var isDisabledForMyCountry = APPCONFIG.subscriptions.disabledCountries.hasOwnProperty(Origin.locale.threeLetterCountryCode());
            if (isDisabledForMyCountry && !comingSoon) {
                $state.go('app.store.wrapper.origin-access-coming-soon', {}, {'location': 'replace', 'reload': true});
                return Promise.reject();
            } else if (!isDisabledForMyCountry && comingSoon) {
                $state.go('app.store.wrapper.origin-access', {}, {'location': 'replace', 'reload': true});
                return Promise.reject();
            } else {
                return Promise.resolve();
            }
        }];
    }

    angular
        .module('origin-routing', ['ui.router', 'origin-components'])
        //constant that provides common functions some routes need. In this way we can avoid code duplication
        //note OriginConfigInjector must be a constant
        .constant('OriginConfigInjector', {
            'resolveTemplate': resolveTemplate,
            'waitForGamesData': waitForGamesData,
            'waitForAuthReady': waitForAuthReady,
            'waitForDictionary': waitForDictionary,
            'checkForOgdRedirect': checkForOgdRedirect,
            'isCountrySubscriptionEnabled': isCountrySubscriptionEnabled
        })
        //Please keep this constant updated with enum in
        //components\src\directives\shell\menu\scripts\item.js
        //components\src\directives\shell\menu\scripts\popoutmenuitem.js
        //components\src\directives\shell\menu\scripts\submenuitem.js
        //navigation categories are used to highlight navs on left hand side when user goes to a url/selects a nav
        .constant('NavigationCategories', {
            'store': 'store',
            'browse': 'browse',
            'freegames': 'freegames',
            'categorypage' : 'categorypage',
            'dealcenter': 'dealcenter',
            'originaccess': 'originaccess',
            'gamelibrary': 'gamelibrary',
            'settings': 'settings',
            'search': 'search',
            'profile': 'profile',
            'home': 'home',
            'homeloggedin': 'homeloggedin',
            'error': 'error'
        })
        .constant('OriginPassThroughQueryParams', orderPassThroughQueryParams(['cmsstage', 'env', 'config', 'enablesocial',
            'showcomponentnames', 'showmissingstrings', 'showmissingstringsfallback', 'usefixturefile', 'automation', 'oigcontext', 'tealiumenv', 'showlogging', 'hidestaticshell']))
        .config(disableStrictMode)
        .config(setupRewrites)
        .run(initEventHandlers)
        .run(listenForAuthChangeForRouteReload);
}());
