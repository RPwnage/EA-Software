'use strict';

describe('Directive: OwnedGameTile', function() {

    var scope,
        compile,
        q,
        deferred,
        ofrDeferred,
        sce,
        callbacks1 = [],
        callbacks2 = [];

    // load the controller's module
    beforeEach(module('origin-components'));

    //
    //      Mock Origin
    // ---------------------------

    window.Origin = {
        feeds: {
            retrieveGameData: function() {
                ofrDeferred = q.defer();
                return ofrDeferred.promise;
            }
        },
        locale: {
            locale: function() {
                return 'en_US';
            }
        }
    };

    spyOn(window.Origin.feeds, 'retrieveGameData').and.callThrough();

    //
    //      Mock Data
    // ---------------------------

    var mock = (function() {
        return {
            data: {
                dictionary: {
                    '%friends% friends are playing %game% right now. This tile is way taller than the other tiles. By a lot. Like way bigger.': '%friends% friends are playing %game% right now. This tile is way taller than the other tiles. By a lot. Like way bigger.',
                    'Play Now': 'Play Now'
                },
                locale: 'en_US',
                catalogInfo: {
                    '12345': {
                        'displayName': 'google'
                    },
                    '6789': {
                        'displayName': 'yahoo'
                    }
                },
                feedGameData: {
                    url: 'http://images.google.com'
                }
            },
            factories: {
                gameData: {
                    getCatalogInfo: function() {
                        deferred = q.defer();
                        return deferred.promise;
                    }
                },
                story: {
                    events: {
                        on: function(eventName, fn) {
                            callbacks1.push(fn);
                        }
                    },
                    getFeedData: function() {
                        // why is this API different?
                        return '12345';
                    },
                    getFeedId: function() {
                        return '1';
                    },
                    getDefaultStr: function() {
                        return 'burp';
                    }
                },
                clientData: {
                    events: {
                        on: function() {}
                    },
                    getGame: function() {
                        return {};
                    }
                },
                socialData: {
                    friendsWhoArePlaying: function() {
                        return [{}, {}, {}];
                    },
                    events: {
                        on: function(eventName, fn) {
                            callbacks2.push(fn);
                        }
                    }
                }
            }
        };

    }());


    //
    //      Spy On Async stuff
    // ---------------------------

    spyOn(mock.factories.gameData, 'getCatalogInfo').and.callThrough();

    //
    //      Inject factories
    // ---------------------------

    beforeEach(module(function($provide) {
        $provide.factory('GamesDataFactory', function() { return mock.factories.gameData; });
        $provide.factory('FeedFactory', function() { return mock.factories.story; });
        $provide.factory('SocialDataFactory', function() { return mock.factories.socialData; });
        $provide.factory('ClientDataFactory', function() { return mock.factories.clientData; });
    }));


    describe('template', function() {

        var directiveName = 'origin-owned-game-tile';

        // load the cached templates
        beforeEach(module('templates'));

        // create the element
        beforeEach(inject(function($compile, $rootScope, $q, $sce, LocFactory) {

            // store the injected vars
            scope = $rootScope.$new();
            compile = $compile;
            q = $q;
            sce = $sce;

            // kill yer callbacks
            callbacks1 = [];
            callbacks2 = [];

            // init yer localization (this should actually only be done once)
            LocFactory.init(mock.data.locale, mock.data.dictionary);

        }));

        /**
        * Generate an element
        * @param {Object} attrs - an object of attributes
        * @method genElm
        */
        function genElm(attrs) {

            var _attrs = [], elm, compiled;

            // generate attributes
            if (typeof(attrs) !== 'undefined') {
                for (var key in attrs) {
                    if (attrs.hasOwnProperty(key)) {
                        _attrs.push(key + '="' + attrs[key]+ '"');
                    }
                }
            }

            //create element
            elm = angular.element('<' + directiveName + ' ' + _attrs.join(' ') + '></' + directiveName +'>');
            compiled = compile(elm)(scope);
            scope.$digest();

            // resolve the promise for the catalog info
            deferred.resolve(mock.data.catalogInfo);
            ofrDeferred.resolve(mock.data.feedGameData);
            scope.$digest();

            return compiled;
        }

        it('should have data in the bound in the isolated scope', function() {
            var compiled = genElm({
                'type': 'A',
                'index': 0
            });
            var isolatedScope = compiled.isolateScope();
            expect(isolatedScope.art).toBe('http://images.google.com');
            expect(isolatedScope.offerId).toBe('12345');
            expect(isolatedScope.name).toBe('google');
            expect(isolatedScope.cta).toBe('Play Now');
            expect(isolatedScope.friendsCount).toBe(3);
            expect(sce.getTrustedHtml(isolatedScope.description)).toBe('burp');
        });

        it('should have a different description based on HT_01', function() {
            var compiled = genElm({
                'type': 'HT_01',
                'index': 0
            });
            var isolatedScope = compiled.isolateScope();
            expect(isolatedScope.art).toBe('http://images.google.com');
            expect(isolatedScope.offerId).toBe('12345');
            expect(isolatedScope.name).toBe('google');
            expect(isolatedScope.cta).toBe('Play Now');
            expect(isolatedScope.friendsCount).toBe(3);
            expect(sce.getTrustedHtml(isolatedScope.description)).toBe('3 friends are playing <a href="#" class="otka">google</a> right now. This tile is way taller than the other tiles. By a lot. Like way bigger.');
        });

        it('should store the right data coming from attributes', function() {
            var compiled = genElm({
                'index': 0,
                'type': 'A',
                'cta': 'Buy Now',
                'description': 'I am a description'
            });
            var isolatedScope = compiled.isolateScope();
            expect(isolatedScope.index).toBe('0');
            expect(isolatedScope.type).toBe('A');
            expect(isolatedScope.ctaStr).toBe('Buy Now');
            expect(isolatedScope.cta).toBe('Buy Now');
            expect(isolatedScope.descriptionStr).toBe('I am a description');
        });

        it('should render properly', function() {
            var compiled = genElm({
                    'type': 'A',
                    'index': 0
                }),
                elm = compiled[0],
                img = elm.querySelector('.origin-tile > .origin-tile-img > img'),
                desc = elm.querySelector('.origin-tile > .origin-tile-content > p');

            expect(angular.element(img).attr('src')).toBe('http://images.google.com');
            expect(angular.element(img).attr('alt')).toBe('google');
            expect(angular.element(desc).html()).toBe('burp');
        });


        it('should handle a story factory update properly', function() {
            var compiled = genElm({
                'type': 'A',
                'index': 0
            });

            // fire the update from the events
            for (var i=0; i<callbacks1.length; i++) {
                callbacks1[i]('6789');
            }

            deferred.resolve(mock.data.catalogInfo);
            ofrDeferred.resolve(mock.data.feedGameData);
            scope.$digest();

            var isolatedScope = compiled.isolateScope();

            expect(isolatedScope.art).toBe('http://images.google.com');
            expect(isolatedScope.offerId).toBe('6789');
            expect(isolatedScope.name).toBe('yahoo');
            expect(isolatedScope.cta).toBe('Play Now');
            expect(isolatedScope.friendsCount).toBe(3);
            expect(sce.getTrustedHtml(isolatedScope.description)).toBe('burp');

        });

        it('should handle a social data factory update properly', function() {
            var compiled = genElm({
                'type': 'HT_01',
                'index': 0
            });

            // fire the update from the events
            for (var i=0; i<callbacks2.length; i++) {
                callbacks2[i]([{}, {}, {}, {}]);
            }

            var isolatedScope = compiled.isolateScope();

            expect(isolatedScope.friendsCount).toBe(4);
            expect(sce.getTrustedHtml(isolatedScope.description)).toBe('4 friends are playing <a href="#" class="otka">google</a> right now. This tile is way taller than the other tiles. By a lot. Like way bigger.');

        });


    });

});
