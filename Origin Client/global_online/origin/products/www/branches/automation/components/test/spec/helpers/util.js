/**
 * UtilFactory global helper functions
 */

'use strict';

describe('UtilFactory - helper functions', function() {
    var utilFactory, LocFactory;

    beforeEach(function () {
        window.OriginComponents = {};
        module('origin-components');

        module(function($provide){

            $provide.factory('LocFactory', function() {
                return {
                    events: {
                        'on': function() {}
                    },
                    'trans': function() {}
                };
            });

        });
    });

    beforeEach(inject(function (_UtilFactory_, _LocFactory_) {
        utilFactory = _UtilFactory_;
        LocFactory = _LocFactory_;
    }));

    describe('buildTag', function() {
        it('will build a jqlite dom element given an element name and attributes', function() {
            var result = utilFactory.buildTag('origin-test-directive', {
                'title': 'My title',
                'hasquotes': '"This is a quotation"',
                'numbers': 1
            });

            expect(result.outerHTML).toBe('<origin-test-directive title="My title" hasquotes="&quot;This is a quotation&quot;" numbers="1"></origin-test-directive>');
        });

        it('will pass along functional/javascript sensitive values by reference', function() {
            var data = {
                obj: {
                    foo: 'bar',
                    func: function(){}
                },
                func: function() {},
                arr: ['item1', 'item2']
            };

            expect(typeof(utilFactory.buildTag('javascript-fu', data))).toBe('object');
        });

        it('will allow users to embed dubious html without breaking the outerHTML', function() {
            var data = {
                gamename: '\' \" ><Battlefield 3™ messages'
            };

            expect(utilFactory.buildTag('origin-wat', data).outerHTML).toEqual('<origin-wat gamename="\' &quot; &gt;&lt;Battlefield 3™ messages"></origin-wat>');
        });
    });

    describe('getFormattedTimeStr', function() {
        it('will format a time into an north american format', function() {
            var currentMillisSinceEpoch = +new Date(),
                isNorthAmericanTimeFormat = /^(0?[1-9]|1[012])(:[0-5]\d) [APap][mM]$/.test(utilFactory.getFormattedTimeStr(currentMillisSinceEpoch));

            expect(isNorthAmericanTimeFormat).toBe(true);
        });
    });

    describe('htmlToScopeAttribute', function() {
        it('should leave non-dashed strings alone', function() {
            expect(utilFactory.htmlToScopeAttribute('foo')).toBe('foo');
            expect(utilFactory.htmlToScopeAttribute('')).toBe('');
            expect(utilFactory.htmlToScopeAttribute('fooBar')).toBe('fooBar');
        });

        it('should covert dash-separated strings to camel case', function() {
            expect(utilFactory.htmlToScopeAttribute('foo-bar')).toBe('fooBar');
            expect(utilFactory.htmlToScopeAttribute('foo-bar-baz')).toBe('fooBarBaz');
            expect(utilFactory.htmlToScopeAttribute('foo:bar_baz')).toBe('fooBarBaz');
        });

        it('should covert browser specific css properties', function() {
            expect(utilFactory.htmlToScopeAttribute('-moz-foo-bar')).toBe('MozFooBar');
            expect(utilFactory.htmlToScopeAttribute('-webkit-foo-bar')).toBe('webkitFooBar');
            expect(utilFactory.htmlToScopeAttribute('-webkit-foo-bar')).toBe('webkitFooBar');
        });

        it('should treat numbers with no extra characters', function() {
            expect(utilFactory.htmlToScopeAttribute('origin-attribute-1')).toBe('originAttribute1');
            expect(utilFactory.htmlToScopeAttribute('origin-attribute1')).toBe('originAttribute1');
            expect(utilFactory.htmlToScopeAttribute('1-origin-attribute')).toBe('1OriginAttribute');
            expect(utilFactory.htmlToScopeAttribute('origin1value')).toBe('origin1value');
        });
    });

    describe('throttle', function() {
        it('should throttle a function', function(done) {
            var callCount = 0,
                throttled = utilFactory.throttle(function(){ callCount++; }, 32);

            throttled();
            throttled();
            throttled();

            var lastCount = callCount;

            setTimeout(function() {
                expect(callCount > lastCount).toBe(true);
                done();
            }, 64);
        });

        it('should not trigger a trailing call when invoked once', function(done) {
            var callCount = 0,
                throttled = utilFactory.throttle(function() { callCount++; }, 32);

            throttled();
            expect(callCount).toBe(1);

            setTimeout(function() {
                expect(callCount).toBe(1);
                done();
            }, 64);
        });

        it('should use a default delay', function() {
            var fn = function() {},
                throttled = utilFactory.throttle(fn);

            expect(throttled).not.toThrow();
        });
    });

    describe('dropThrottle', function() {
        it('it will immediately invoke the callback and discard subsequent calls to the callback function until the interval has passed', function(done) {
            var callCount = 0,
                dropThrottled = utilFactory.dropThrottle(function() {
                    callCount++;
                }, 32);

            dropThrottled();
            dropThrottled();
            dropThrottled();

            setTimeout(function(){
                expect(callCount).toBe(1);
                done();
            }, 64);
        });

        it('should use a default delay', function() {
            var fn = function() {},
                dropThrottled = utilFactory.dropThrottle(fn);

            expect(dropThrottled).not.toThrow();
        });
    });

    describe('reverseThrottle', function() {
        it('should invoke the callback after the defined period', function(done) {
            var callCount = 0,
                reverseThrottled = utilFactory.reverseThrottle(function() {
                    callCount++;
                }, 32);

            reverseThrottled();
            expect(callCount).toBe(0);

            setTimeout(function() {
                expect(callCount).toBe(1);
                done();
            }, 64);
        });

        it('should cancel and restart the reverseThrottle timer if invoked again before the timeout', function(done) {
            var callCount = 0,
                reverseThrottled = utilFactory.reverseThrottle(function() {
                    callCount++;
                }, 100);

            reverseThrottled();

            setTimeout(function() {
                reverseThrottled();
                setTimeout(function() {
                    expect(callCount).toBe(0);
                    done();
                }, 50);
            }, 75);
        });

        it('should use a default delay', function() {
            var fn = function() {},
                reverseThrottled = utilFactory.reverseThrottle(fn);

            expect(reverseThrottled).not.toThrow();
        });
    });

    describe('guid', function() {
        it('will be a string of the form XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX where x is a hex number', function() {
            var isValidGuidFormat = /^[0-9A-F]{8}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{4}-[0-9A-F]{12}$/i.test(utilFactory.guid());

            expect(isValidGuidFormat).toBe(true);
        });
    });

    describe('formatBytes', function() {
        it('will automatically format byte sizes to a fixed precision and converted to a string', function() {
            expect(utilFactory.formatBytes(0)).toEqual('0.00');
            expect(utilFactory.formatBytes(512)).toEqual('512.00');
            expect(utilFactory.formatBytes(450029)).toEqual('439.48');
            expect(utilFactory.formatBytes(2000903)).toEqual('1.91');
            expect(utilFactory.formatBytes(50203900)).toEqual('47.88');
            expect(utilFactory.formatBytes(2039020029)).toEqual('1.90');
            expect(utilFactory.formatBytes(8290239009022)).toEqual('7.54');
        });
    });

    describe('replaceAll', function() {
        it('will replace all occurences in a string with the given search value', function() {
            var replacements = {
                    '昨夜のコンサートは最高でした': 'japanese 昨夜, 最高',
                    '\'': 'ascii',
                    '`': '&#96;'
                },
                text = '昨夜のコンサートは最高でした \' ``` 昨夜のコンサートは最高でした',
                expected = 'japanese 昨夜, 最高 ascii &#96;&#96;&#96; japanese 昨夜, 最高';

            expect(utilFactory.replaceAll(text, replacements)).toEqual(expected);
        });

        it('will pass through the text if a map is not provided', function() {
            var text = 'foobar';

            expect(utilFactory.replaceAll(text)).toEqual('foobar');
        });

        it('will pass through the text if the map is empty', function() {
            var text = 'bazbat';

            expect(utilFactory.replaceAll(text, {})).toEqual('bazbat');
        });

        it('will only process strings', function(){
            var replacements = {
                    'bazbat': 'foobar',
                },
                text = ['bazbat'];

            expect(utilFactory.replaceAll(text, replacements)).toEqual(['bazbat']);
        });
    });

    describe('unwrapPromiseAndCall', function () {
        it('will unwrap a promise and call a callback', function(done) {
            function successFunc(data) {
                expect(data).toBe('foo');

                done();
            }

            utilFactory.unwrapPromiseAndCall(successFunc)(Promise.resolve('foo'));
        });

        it('will unwrap a promise.all and call a callback', function(done) {
            function successFunc(data1, data2) {
                expect(data1).toBe('data set 1');
                expect(data2).toBe('data set 2');

                done();
            }

            utilFactory.unwrapPromiseAndCall(successFunc)(Promise.resolve(['data set 1', 'data set 2']));
        });

        it('will handle errors with an error func', function(done) {
            var errorCode = new Error('rejected!');

            function errorFunc(err) {
                expect(err).toEqual(errorCode);

                done();
            }

            utilFactory.unwrapPromiseAndCall(undefined, errorFunc)(Promise.reject(errorCode));
        });

        it('will handle non functions gracefully', function() {
            expect(utilFactory.unwrapPromiseAndCall()()).toBeUndefined();
        });
    });

    describe('getLocalizedEditionName', function () {
        it('will translate an edition name', function() {
            spyOn(LocFactory, 'trans').and.returnValue('Le Standard Edition');

            expect(utilFactory.getLocalizedEditionName('Standard Edition')).toEqual('Le Standard Edition');
        });


        it('will not translate an edition name if no translation exists', function() {
            spyOn(LocFactory, 'trans').and.returnValue('');

            expect(utilFactory.getLocalizedEditionName('Standard Edition')).toEqual('Standard Edition');
        });
    });

    describe('isValidNucleusId', function() {
        it('will return true when nucluesId is "123456"', function () {
            expect(utilFactory.isValidNucleusId('123456')).toBeTruthy();
        });

        it('will return false when nucluesId is 123456', function () {
            expect(utilFactory.isValidNucleusId(123456)).toBeTruthy();
        });

        it('will return false when nucluesId is 1a23456', function () {
            expect(utilFactory.isValidNucleusId('1a23456')).toBeFalsy();
        });

        it('will return false when nucluesId is abc', function () {
            expect(utilFactory.isValidNucleusId('abc')).toBeFalsy();
        });

        it('will return false when nucluesId is null', function () {
            expect(utilFactory.isValidNucleusId(null)).toBeFalsy();
        });

        it('will return false when nucluesId is undefined', function () {
            expect(utilFactory.isValidNucleusId(undefined)).toBeFalsy();
        });

    });
});
