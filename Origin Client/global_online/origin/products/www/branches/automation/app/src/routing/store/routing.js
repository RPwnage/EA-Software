/**
 * Routing for the store application
 */
(function () {
    'use strict';


    function routing($stateProvider, OriginConfigInjector, NavigationCategories) {

        $stateProvider
            .state('app.store', {
                url: '/store',
                abstract: true,
                data: {
                    section: NavigationCategories.store
                },
                views: {
                    'content@': {
                        templateUrl: '/views/store.html',
                        resolve: {
                            waitForGamesData: OriginConfigInjector.waitForGamesData(),
                            waitForAuthReady: OriginConfigInjector.waitForAuthReady()
                        }
                    }
                }
            })
            .state('app.store.wrapper', {
                views: {
                    'storewrapper': {
                        template: '<div id="storecontent" ui-view="storecontent"></div>'
                    },
                    'storemod': {
                        templateProvider: OriginConfigInjector.resolveTemplate('sitestripe')
                    },
                    // Render the client footer on store pages if on the client
                    // otherwise render the web footer.
                    // The '@' sign is to target the 'footer' ui-view in the root template.
                    'footer@': {
                        templateProvider: function (TemplateResolver, $stateParams) {
                            var templateKey = Origin.client.isEmbeddedBrowser() ? 'footer-client' : 'footer-web';
                            return TemplateResolver.getTemplate(templateKey, $stateParams);
                        }
                    }
                }
            })
            .state('app.store.wrapper.about', {
                url: '/about',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('about')
                    }
                }
            })
            .state('app.store.wrapper.vat', {
                url: '/vat',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('vat')
                    }
                }
            })
            .state('app.store.wrapper.download', {
                url: '/download',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('download')
                    }
                }
            })
            .state('app.store.wrapper.ggg', {
                url: '/great-game-guarantee',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('great-game-guarantee')
                    }
                }
            })
            .state('app.store.wrapper.ggg-policy', {
                url: '/great-game-guarantee-terms',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('great-game-guarantee-terms')
                    }
                }
            })
            .state('app.store.wrapper.promo-terms', {
                url: '/promo-terms?:id',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('promo-terms')
                    }
                }
            })
            .state('app.store.wrapper.browse', {
                url: '/browse?searchString&:fq&:sort',
                data: {
                    section: NavigationCategories.browse
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('browse')
                    }
                }
            })
            .state('app.store.wrapper.home', {
                url: '',
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('storehome')
                    }
                }
            })
            .state('app.store.wrapper.categorypage', {
                url: '/shop/:categoryPageId',
                data: {
                    section: NavigationCategories.categorypage
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('categorypage')
                    }
                }
            })
            .state('app.store.wrapper.freegames', {
                url: '/free-games',
                data: {
                    section: NavigationCategories.freegames
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('freegames-hub')
                    }
                }
            })
            .state('app.store.wrapper.onthehouse', {
                url: '/free-games/on-the-house',
                data: {
                    section: NavigationCategories.freegames
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('freegames-onthehouse')
                    }
                }
            })
            .state('app.store.wrapper.gametime', {
                url: '/free-games/trials',
                data: {
                    section: NavigationCategories.freegames
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('freegames-gametime')
                    }
                }
            })
            .state('app.store.wrapper.demosbetas', {
                url: '/free-games/demos-and-betas',
                data: {
                    section: NavigationCategories.freegames
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('freegames-demosbetas')
                    }
                }
            })
            .state('app.store.wrapper.origin-access', {
                url: '/origin-access',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-landing-page'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(false)
                        },
                        'controller': 'LandingCtrl'
                    }
                }
            })
            .state('app.store.wrapper.origin-access-coming-soon', {
                url: '/origin-access/coming-soon',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-coming-soon'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(true)
                        }
                    }
                }
            })
            .state('app.store.wrapper.origin-access-vault', {
                url: '/origin-access/vault-games',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-vault'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(false)
                        }
                    }
                }
            })
            .state('app.store.wrapper.origin-access-trials', {
                url: '/origin-access/trials',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-trials'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(false)
                        }
                    }
                }
            })
            .state('app.store.wrapper.origin-access-faq', {
                url: '/origin-access/faq',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-faq'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(false)
                        }
                    }
                }
            })
            .state('app.store.wrapper.origin-access-terms', {
                url: '/origin-access/terms',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-terms'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(false)
                        }
                    }
                }
            })
            .state('app.store.wrapper.origin-access-real-deal', {
                url: '/origin-access/real-deal',
                data: {
                    section: NavigationCategories.originaccess
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('oax-real-deal'),
                        resolve: {
                            checkCountry: OriginConfigInjector.isCountrySubscriptionEnabled(false)
                        }
                    }
                }
            })
            .state('app.store.wrapper.deals-center', {
                url: '/deals',
                data: {
                    section: NavigationCategories.dealcenter
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('deals-center')
                    }
                }
            })
            .state('app.store.wrapper.bundle', {
                url: '/deals/:bundle?searchString&fq&sort',
                data: {
                    section: NavigationCategories.dealcenter
                },
                views: {
                    'storecontent': {
                        templateProvider: OriginConfigInjector.resolveTemplate('bundle')
                    }
                }
            });
    }

    angular.module('origin-routing')
        .config(routing);
})();
