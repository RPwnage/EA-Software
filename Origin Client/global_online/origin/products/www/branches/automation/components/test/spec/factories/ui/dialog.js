/**
 * Dialog factory - a modal generator
 */

'use strict';

describe('DialogFactory', function() {
    var dialogFactory;

    beforeEach(function () {
        window.OriginComponents = {};
        window.Origin = {
            'utils': {
                'Communicator': function() {}
            }
        };

        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    log: function(){}
                };
            });

            $provide.factory('StackFactory', function() {
                return {
                    make: function() {}
                };
            });

            $provide.factory('NavigationFactory', function() {
                return {
                };
            });
        });
    });

    beforeEach(inject(function (_DialogFactory_) {
        dialogFactory = _DialogFactory_;
    }));

    describe('buildHtml', function() {
        it('will create a single tag', function() {
            var simpleDialog = [{
                data: {
                    class : 'origin-dialog-content',
                    defaultbtn: 'no',
                    header: 'abc123',
                    id: 'ogd-violator-read-more-dialog',
                    nobtntext: 'Close',
                },
                name: 'origin-dialog-content-prompt'
            }];

            expect(dialogFactory.buildHtml(simpleDialog)).toEqual('<origin-dialog-content-prompt class=\'origin-dialog-content\' defaultbtn=\'no\' header=\'abc123\' id=\'ogd-violator-read-more-dialog\' nobtntext=\'Close\'></origin-dialog-content-prompt>');
        });

        it('will create a nested tag', function() {
            var complexDialog = [{
                data: {
                    id: 'ogd-violator-read-more-dialog',
                },
                name: 'origin-dialog-content-prompt',
                directives: [{
                    data: {
                        subheader: 'foobaz'
                    },
                    name: 'foo-baz-header'
                }]
            }];

            expect(dialogFactory.buildHtml(complexDialog)).toEqual('<origin-dialog-content-prompt id=\'ogd-violator-read-more-dialog\'><foo-baz-header subheader=\'foobaz\'></foo-baz-header></origin-dialog-content-prompt>');
        });

        it('will create a nested tag with multiple subdirectives', function() {
            var complexDialog = [{
                data: {
                    id: 'ogd-violator-read-more-dialog',
                },
                name: 'origin-dialog-content-prompt',
                directives: [{
                    data: {
                        subheader: 'foobaz'
                    },
                    name: 'foo-baz-header'
                }, {
                    data: {
                        subheader: 'bazbat'
                    },
                    name: 'baz-bat-header',
                    directives: [{
                        data: {
                            'title-str': 'level 3'
                        },
                        name: 'level-3-tag'
                    }]
                }]
            }];

            expect(dialogFactory.buildHtml(complexDialog)).toEqual('<origin-dialog-content-prompt id=\'ogd-violator-read-more-dialog\'><foo-baz-header subheader=\'foobaz\'></foo-baz-header><baz-bat-header subheader=\'bazbat\'><level-3-tag title-str=\'level 3\'></level-3-tag></baz-bat-header></origin-dialog-content-prompt>');
        });

        it('will encode javascript object and array references as json', function() {
            var dialog = [{
                data: {
                    obj: {'foo': 'bar'},
                    arr: ['foo', 'bar']
                },
                name: 'javascript-to-json'
            }];

            expect(dialogFactory.buildHtml(dialog)).toEqual('<javascript-to-json obj=\'{"foo":"bar"}\' arr=\'["foo","bar"]\'></javascript-to-json>');
        });

        it('will escape backticks and single quotes', function() {
            var dialog = [{
                data: {
                    wat: 'Battlefield 3™ \'" `<a href="http://www.example.com">go to e\'xamples!</a>messages'
                },
                name: 'escaping-wat'
            }];

            expect(dialogFactory.buildHtml(dialog)).toEqual('<escaping-wat wat=\'Battlefield 3™ &#39;" &#96;<a href="http://www.example.com">go to e&#39;xamples!</a>messages\'></escaping-wat>');
        });

        it('will not build a tag if the name field is missing', function() {
            var corruptDialog = [{
                data: {
                    id: 'foobroken'
                }
            }];

            expect(dialogFactory.buildHtml(corruptDialog)).toEqual('');
        });

        it('Will fail gracefully if the input format is undefined/missing', function(){
            var corruptDialog;

            expect(dialogFactory.buildHtml(corruptDialog)).toEqual('');
        });
    });
});