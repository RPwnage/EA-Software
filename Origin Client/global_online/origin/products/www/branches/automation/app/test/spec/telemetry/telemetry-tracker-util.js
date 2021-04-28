/**
 * Telemetry Tracker Util
 */

'use strict';

describe('Telemetry Tracker Util', function() {
    var telemetryTrackerUtil;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            telemetry: {
                sendMarketingEvent: function(){}
            }
        };

        angular.mock.module('originApp');

        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    debug: function(){}
                };
            });
        });
    });

    beforeEach(inject(function(_TelemetryTrackerUtil_) {
        telemetryTrackerUtil = _TelemetryTrackerUtil_;
    }));

    describe('sendTelemetryEvent()', function() {
        beforeEach(function() {
            spyOn(window.Origin.telemetry, 'sendMarketingEvent');
        });

        it('will send a telemetry event with payload', function() {
            var payload = {};
            telemetryTrackerUtil.sendTelemetryEvent(payload);
            expect(window.Origin.telemetry.sendMarketingEvent).toHaveBeenCalled();
        });

        it('will not send a telemetry event without payload', function() {
            telemetryTrackerUtil.sendTelemetryEvent();
            expect(window.Origin.telemetry.sendMarketingEvent).not.toHaveBeenCalled();
        });
    });
});