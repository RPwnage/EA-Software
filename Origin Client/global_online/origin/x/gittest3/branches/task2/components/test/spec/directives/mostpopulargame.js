'use strict';

describe('Directive: MostPopularGameTile', function() {

    var scope,
        compile,
        q,
        deferred,
        sce,
        callback;

    // load the controller's module
    beforeEach(module('origin-components'));

    //
    //      Mock Data
    // ---------------------------

    var mock = (function() {
        return {
            data: {
                dictionary: {
                    '%game% is the most popular game this week!': '%game% is the most popular game this week!',
                    'Buy Now': 'Buy Now'
                },
                locale: 'en_US',
                catalogInfo: {
                    '12345': {
                        'displayName': 'google'
                    },
                    '6789': {
                        'displayName': 'yahoo'
                    }
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
                            callback = fn;
                        }
                    },
                    getFeedData: function() {
                        return {
                            'imageUrl': 'http://images.google.com',
                            'offerId': '12345'
                        };
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
                        return [];
                    },
                    events: {
                        on: function() {}
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

        var directiveName = 'origin-most-popular-game-tile';

        // load the cached templates
        beforeEach(module('templates'));

        // create the element
        beforeEach(inject(function($compile, $rootScope, $q, $sce, LocFactory) {
            scope = $rootScope.$new();
            compile = $compile;
            q = $q;
            sce = $sce;
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
            scope.$digest();

            return compiled;
        }

        it('should have data in the bound in the isolated scope', function() {
            var compiled = genElm();
            var isolatedScope = compiled.isolateScope();
            expect(isolatedScope.art).toBe('http://images.google.com');
            expect(isolatedScope.offerId).toBe('12345');
            expect(isolatedScope.data.displayName).toBe('google');
            expect(isolatedScope.cta).toBe('Buy Now');
            expect(sce.getTrustedHtml(isolatedScope.description)).toBe('<a href="#" class="otka">google</a> is the most popular game this week!');
        });

        it('should store the right data coming from attributes', function() {
            var compiled = genElm({
                'index': '0',
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
            var compiled = genElm(),
                elm = compiled[0],
                img = elm.querySelector('.origin-tile > .origin-tile-img > img'),
                desc = elm.querySelector('.origin-tile > .origin-tile-content > p');

            expect(angular.element(img).attr('src')).toBe('http://images.google.com');
            expect(angular.element(desc).html()).toBe('<a href="#" class="otka">google</a> is the most popular game this week!');
        });

        it('should handle a story factory update properly', function() {
            var compiled = genElm();

            // fire the update from the events
            callback({
                'imageUrl': 'http://images.yahoo.com',
                'offerId': '6789'
            });

            deferred.resolve(mock.data.catalogInfo);
            scope.$digest();

            var isolatedScope = compiled.isolateScope();

            expect(isolatedScope.art).toBe('http://images.yahoo.com');
            expect(isolatedScope.offerId).toBe('6789');
            expect(isolatedScope.data.displayName).toBe('yahoo');
            expect(isolatedScope.cta).toBe('Buy Now');
            expect(sce.getTrustedHtml(isolatedScope.description)).toBe('<a href="#" class="otka">yahoo</a> is the most popular game this week!');
        });

    });

});
