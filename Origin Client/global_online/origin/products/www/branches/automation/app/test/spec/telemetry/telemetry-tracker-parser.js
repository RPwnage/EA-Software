/**
 * Telemetry Tracker Parser
 */

'use strict';

describe('Telemetry Tracker Parser', function() {
    var telemetryTrackerParser;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {};

        angular.mock.module('originApp');

        module(function($provide) {
            $provide.factory('$state', function() {
                return {
                    current: {
                        name: 'mock.state.name'
                    }
                };
            });

            $provide.factory('TelemetryTrackerUtil', function() {
                return {
                    sendTelemetryEvent: function(){}
                };
            });
        });
    });

    beforeEach(inject(function(_TelemetryTrackerParser_) {
        telemetryTrackerParser = _TelemetryTrackerParser_;
    }));

    describe('getAttribute()', function() {
        it('is an attribute', function() {
            var str = '[attribute]';
            expect(telemetryTrackerParser.getAttribute(str)).toBe('attribute');
        });

        it('is missing opening attribute bracket', function() {
            var str = 'attribute]';
            expect(telemetryTrackerParser.getAttribute(str)).toBe(undefined);
        });

        it('is missing closing attribute bracket', function() {
            var str = '[attribute';
            expect(telemetryTrackerParser.getAttribute(str)).toBe(undefined);
        });

        it('is not an attribute', function() {
            var str = 'Not an attribute';
            expect(telemetryTrackerParser.getAttribute(str)).toBe(undefined);
        });
    });

    describe('getSelector()', function() {
        it('is a selector without attribute', function() {
            var str = '.my .selectors';
            expect(telemetryTrackerParser.getSelector(str)).toBe('');
        });

        it('is a selector with attribute', function() {
            var str = '.my .selectors[attribute]';
            expect(telemetryTrackerParser.getSelector(str)).toBe('.my .selectors');
        });
    });

    describe('isGlobalEvent()', function() {
        it('is global event', function() {
            var config = {
                stateName: 'mock.state.name'
            };
            expect(telemetryTrackerParser.isGlobalEvent(config)).toBe(false);
        });

        it('is not global event', function() {
            var config = {};
            expect(telemetryTrackerParser.isGlobalEvent(config)).toBe(true);
        });
    });

    describe('isMatchingState()', function() {
        it('is matching state', function() {
            var stateName = 'mock.state.name';
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBe(true);
        });

        it('is matching part of the state', function() {
            var stateName = 'mock.state';
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBe(true);
        });

        it('is not matching empty state', function() {
            var stateName = '';
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBeFalsy();
        });

        it('is not matching null state', function() {
            var stateName = null;
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBeFalsy();
        });

        it('is not matching state', function() {
            var stateName = 'unmatching.state.name';
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBeFalsy();
        });

        it('is matching state for array of states', function() {
            var stateName = ['mock.state.name', 'mock.state.name2'];
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBe(true);
        });

        it('is not matching state for empty array of states', function() {
            var stateName = [];
            expect(telemetryTrackerParser.isMatchingState(stateName)).toBeFalsy();
        });
    });

    describe('isToken()', function() {
        it('is a token', function() {
            var str = '{token}';
            expect(telemetryTrackerParser.isToken(str)).toBe(true);
        });

        it('is missing opening token bracket', function() {
            var str = 'token]';
            expect(telemetryTrackerParser.isToken(str)).toBe(false);
        });

        it('is missing closing token bracket', function() {
            var str = '[token';
            expect(telemetryTrackerParser.isToken(str)).toBe(false);
        });

        it('is not a token', function() {
            var str = 'Not a token';
            expect(telemetryTrackerParser.isToken(str)).toBe(false);
        });
    });
});