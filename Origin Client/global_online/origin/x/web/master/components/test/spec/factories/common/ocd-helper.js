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
        });
    });

    var ocdHelper, compile, rootScope;

    beforeEach(inject(function(_OcdHelperFactory_, _$compile_, _$rootScope_) {
            ocdHelper = _OcdHelperFactory_;
            compile = _$compile_;
            rootScope = _$rootScope_;
    }));

    describe('buildTag', function() {
        it('will build a jq dom document given an element name and attributes as an object', function() {
            var result = ocdHelper.buildTag('origin-test-directive', {
                'title': 'My title',
                'hasquotes': '"This is a quotation"',
                'numbers': 1
            });

            expect(result.outerHTML).toBe('<origin-test-directive title="My title" hasquotes="&quot;This is a quotation&quot;" numbers="1"></origin-test-directive>');
        });
    });

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
});