/**
 * Jasmine functional test
 */

'use strict';

describe('OCD Helper Factory', function() {
    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){}
                };
            });

            $provide.factory('SubscriptionFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });

            $provide.factory('GamesDataFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });

            $provide.factory('AuthFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });

            $provide.factory('GamesEntitlementFactory', function() {
                return {
                    events: {
                        'on': function() {},
                        'fire': function() {}
                    }
                };
            });                  
        });
    });

    var ocdHelper, compile, rootScope;

    beforeEach(inject(function(_OcdHelperFactory_, _$compile_, _$rootScope_) {
            ocdHelper = _OcdHelperFactory_;
            compile = _$compile_;
            rootScope = _$rootScope_;
    }));

    describe('slingDataToDom', function() {
        it('will build a dom object from a sling response', function() {
            var data = {
                "origin-game-tile": {
                    "title": "spez bez",
                    "items": [
                        {
                            "origin-game-violator-programmed": {
                                "title": "foo \"bar",
                                "icon": "baz bat",
                                "items": [
                                    {
                                        "origin-nested-violator": {
                                            "data": "foo"
                                        }
                                    }
                                ]
                            }
                        }, {
                            "origin-game-violator-aliens": {
                                "title": "lazy brown dog"
                            }
                        }
                    ]
                }
            };

            var expected = '<origin-game-tile title="spez bez"><origin-game-violator-programmed title="foo &quot;bar" icon="baz bat"><origin-nested-violator data="foo"></origin-nested-violator></origin-game-violator-programmed><origin-game-violator-aliens title="lazy brown dog"></origin-game-violator-aliens></origin-game-tile>';

            expect(ocdHelper.slingDataToDom(data).outerHTML).toBe(expected);
        });
    });

    describe('HTML generator methods', function() {
        it('will compile when passed to the AngularJS $compile function', function(){
            var scope = rootScope.$new();
            var element = compile(ocdHelper.buildTag('foobar', {'bar': 'foo'}))(scope);
            scope.$apply();
            expect(typeof(element)).toBe('object');
        });
    });

    describe('findDirectives helper', function() {
        var ocdData = {
            'gamehub': {
                'components': {
                    'items': [
                        {
                            'origin-foo': {
                                'title': 'abc123'
                            }
                        }, {
                            'origin-cta': {
                                'title': 'foo',
                                'description': 'bar'
                            }
                        }, {
                            'origin-cta': {
                                'title': 'coca',
                                'description': 'koala'
                            }
                        },{
                            'origin-carousel': {
                                'title': 'baz',
                                'description': 'bat',
                                'items': [
                                    {
                                        'origin-bogus': {
                                            'image': 'broken link'
                                        }
                                    }, {
                                        'origin-item': {
                                            'image': 'http://www.example.com/picture-1.jpg'
                                        }
                                    }, {
                                        'origin-item': {
                                            'image': 'http://www.example.com/picture-2.jpg'
                                        }
                                    }
                                ]
                            }
                        }
                    ]
                }
            }
        }

        it('will find multiple directives by string for shallow lookups', function () {
            expect(ocdHelper.findDirectives('origin-cta')(ocdData)).toEqual([
                {
                    'origin-cta': {
                        'title': 'foo',
                        'description': 'bar'
                    }
                }, {
                    'origin-cta': {
                        'title': 'coca',
                        'description': 'koala'
                    }
                }
            ]);
        });

        it('can be used subsequently to search for child directives using a subpath', function() {
            var carousels = ocdHelper.findDirectives('origin-carousel')(ocdData);
            expect(ocdHelper.findDirectives('origin-item', [0, 'origin-carousel', 'items'])(carousels)).toEqual([
                {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-1.jpg'
                    }
                }, {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-2.jpg'
                    }
                }
            ]);
        })
    });

    describe('coalesceChildDirectives helper', function() {
        var ocdData = {
            'gamehub': {
                'components': {
                    'items': [
                        {
                            'origin-carousel': {
                                'title': 'baz',
                                'description': 'bat',
                                'items': [
                                    {
                                        'origin-bogus': {
                                            'image': 'broken link'
                                        }
                                    }, {
                                        'origin-item': {
                                            'image': 'http://www.example.com/picture-1.jpg'
                                        }
                                    }, {
                                        'origin-item': {
                                            'image': 'http://www.example.com/picture-2.jpg'
                                        }
                                    }
                                ]
                            }
                        }, {
                            'origin-carousel': {
                                'title': 'baz',
                                'description': 'bat',
                                'items': [
                                    {
                                        'origin-bogus': {
                                            'image': 'this is another bogus record'
                                        }
                                    }, {
                                        'origin-item': {
                                            'image': 'http://www.example.com/picture-3.jpg'
                                        }
                                    }
                                ]
                            }
                        }, {
                            'origin-deep': {
                                'items': [
                                    {
                                        'origin-carousel': {
                                            'description': 'chee',
                                            'items': [
                                                {
                                                    'origin-item': {
                                                        'image': 'http://www.example.com/picture-4.jpg'
                                                    }
                                                }
                                            ]
                                        }
                                    }, {
                                        'origin-carousel': {
                                            'title': 'choo',
                                            'items': [
                                                {
                                                    'origin-bogus': {
                                                        'image': 'this is another bogus record'
                                                    }
                                                }, {
                                                    'origin-item': {
                                                        'image': 'http://www.example.com/picture-5.jpg'
                                                    }
                                                }
                                            ]
                                        }
                                    }

                                ]
                            }
                        }

                    ]
                }
            }
        };

        it('will glob immediate descendants of the carousel', function () {
            var childDirectiveCollection = ocdHelper.coalesceChildDirectives('origin-carousel', 'origin-item')(ocdData);

            expect(childDirectiveCollection.length).toEqual(3);
            expect(childDirectiveCollection).toEqual([
                {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-1.jpg'
                    }
                }, {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-2.jpg'
                    }
                }, {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-3.jpg'
                    }
                }
            ]);
        });

        it('will glob subitems if a subpath is defined', function() {
            var parentDirectives = ocdHelper.findDirectives('origin-deep')(ocdData),
                childDirectiveCollection = ocdHelper.coalesceChildDirectives('origin-carousel', 'origin-item', [0, 'origin-deep', 'items'])(parentDirectives);

            expect(childDirectiveCollection.length).toEqual(2);
            expect(childDirectiveCollection).toEqual([
                {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-4.jpg'
                    }
                }, {
                    'origin-item': {
                        'image': 'http://www.example.com/picture-5.jpg'
                    }
                }

            ]);
        });
    });

    describe('flattenDirectives helper', function() {
        var data = [
            {
                'origin-foo': {
                    'title': 'abc123',
                    'items': []
                }
            }, {
                'origin-cta': {
                    'title': 'foo',
                    'description': 'bar'
                }
            }
        ];

        it('will return the attibute values of a selected directive', function() {
            expect(ocdHelper.flattenDirectives('origin-foo')(data)).toEqual([
                {
                    'title': 'abc123',
                    'items': []
                }
            ]);
        });
    });

    describe('wrapDocumentForCompile', function() {
        var scope;

        beforeEach(function() {
            scope = rootScope.$new();
        });

        it('will wrap a document fragment with a cq targeting div', function() {
            var fragment = document.createDocumentFragment(),
                ul = document.createElement('ul'),
                li = document.createElement('li'),
                ulLevel = fragment.appendChild(ul),
                liLevel = ulLevel.appendChild(li);

            expect(ocdHelper.wrapDocumentForCompile(fragment).outerHTML).toEqual('<div class="origin-cms-targeting-wrapper"><ul><li></li></ul></div>');
            expect(typeof(ocdHelper.wrapDocumentForCompile(fragment))).toEqual('object');
            var compileDoc = compile(ocdHelper.wrapDocumentForCompile(fragment))(scope);
            expect(typeof(compileDoc)).toBe('object');
        });

        it('will wrap a single new tag in a cq targeting div', function() {
            var tag = ocdHelper.buildTag('foobar', {'bar': 'foo'});

            expect(ocdHelper.wrapDocumentForCompile(tag).outerHTML).toEqual('<div class="origin-cms-targeting-wrapper"><foobar bar="foo"></foobar></div>');
            expect(typeof(ocdHelper.wrapDocumentForCompile(tag))).toEqual('object');
            var compileDoc = compile(ocdHelper.wrapDocumentForCompile(tag))(scope);
            expect(typeof(compileDoc)).toBe('object');
        });

        it('will allow the user to override the default class', function() {
            var tag = ocdHelper.buildTag('foobar', {'bar': 'foo'});

            expect(ocdHelper.wrapDocumentForCompile(tag, 'my-class').outerHTML).toEqual('<div class="my-class"><foobar bar="foo"></foobar></div>');
            expect(typeof(ocdHelper.wrapDocumentForCompile(tag, 'my-class'))).toEqual('object');
            var compileDoc = compile(ocdHelper.wrapDocumentForCompile(tag))(scope);
            expect(typeof(compileDoc)).toBe('object');
        });

        it('will fail gracefully if a document is not provided', function() {
            var tag;

            expect(typeof(ocdHelper.wrapDocumentForCompile(tag))).toEqual('object');
            var compileDoc = compile(ocdHelper.wrapDocumentForCompile(tag))(scope);
            expect(typeof(compileDoc)).toBe('object');
        });
    });
});