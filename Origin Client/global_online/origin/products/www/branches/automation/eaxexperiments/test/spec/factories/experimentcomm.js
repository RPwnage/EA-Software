/**
 * Jasmine functional test
 */

'use strict';

describe('ExperimentCommunicator Factory', function() {
    var experimentCommFactory;

    beforeEach(function() {
        window.OriginComponents = {};

        function Communicator() {}
        Communicator.prototype.fire = function() {};
        Communicator.prototype.on = function () {};
        Communicator.prototype.once = function () {};
        Communicator.prototype.off = function () {};

        angular.mock.module('eax-experiments');

    });

    beforeEach(inject(function(_ExperimentCommFactory_) {
        experimentCommFactory = _ExperimentCommFactory_;
    }));


    it('test simple on and fire', function() {
        var events = new experimentCommFactory.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        events.on('customevent', callback);
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test fire undefined', function() {
        var events = new experimentCommFactory.Communicator();

        expect(events.fire).toThrow(new Error('Communicator.fire: eventName is undefined'));
    });

    it('test on with undefined', function() {
        var events = new experimentCommFactory.Communicator(),
            undef;

        function callback() {

        }

        function foo () {
            events.on(undef, callback);
        }

        expect(foo).toThrow(new Error('Communicator._addEvent: eventName is undefined'));
    });

    it('test off', function() {
        var events = new experimentCommFactory.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // should not fire
        events.on('customevent', callback);
        events.off('customevent', callback);
        events.fire('customevent');

        expect(counter).toEqual(0);
    });

    it('test off undefined', function() {
        var events = new experimentCommFactory.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // should not fire
        events.on('customevent', callback);
        expect(events.off).toThrow(new Error('Communicator.fire: eventName is undefined'));
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test once should only fire once', function() {
        var events = new experimentCommFactory.Communicator(),
            counter = 0;

        function callback() {
            counter++;
        }

        // should only fire once
        events.once('customevent', callback);
        events.fire('customevent');
        events.fire('customevent');

        expect(counter).toEqual(1);
    });

    it('test using detach before event fires', function() {
        var events = new experimentCommFactory.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        handle.detach();
        events.fire('customevent');

        expect(counter).toEqual(0);
    });

    it('test reattaching', function () {
        var events = new experimentCommFactory.Communicator(),
            counter = 0,
            handle;

        function callback() {
            counter++;
        }

        handle = events.on('customevent', callback);
        events.fire('customevent');
        handle.detach();
        events.fire('customevent');
        handle.attach();
        events.fire('customevent');

        expect(counter).toEqual(2);
    });


});