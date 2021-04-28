/**
 * Jasmine functional test
 */

'use strict';

describe('Observable Factory', function() {
    var observableFactory, EventSystemMock;

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

        EventSystemMock = function() {
            this.on = function(){};
            this.off = function(){};
            this.fire = function(){};
        };
    });

    beforeEach(inject(function(_ObservableFactory_) {
        observableFactory = _ObservableFactory_;
    }));

    describe('observe', function() {
        it('Will create a new obervable data container', function(){
            var observable1 = observableFactory.observe({'foo': 'bar'}, EventSystemMock);
            var observable2 = observableFactory.observe({'foo': 'bar'}, EventSystemMock);

            expect(observable1).not.toBe(observable2);
        });

        describe('Observable response object', function() {
            var data, testObservable;

            beforeEach(function() {
                data = {'foo': 'bar'};
                testObservable = observableFactory.observe(data, EventSystemMock);
            });

            it('Will add a callback to the local commit() event listener chain', function() {
                spyOn(testObservable.events, 'on').and.callThrough();

                var callback = function(){return 'foo';};
                testObservable.addObserver(callback);

                expect(testObservable.events.on).toHaveBeenCalledWith('local:change', callback);
            });

            it('Will remove a callback from the local commit() chain', function(){
                spyOn(testObservable.events, 'off').and.callThrough();

                var callback = function(){return 'foo';};
                testObservable.removeObserver(callback);

                expect(testObservable.events.off).toHaveBeenCalledWith('local:change', callback);
            });

            it('Will fire the local commit event chain', function(){
                spyOn(testObservable.events, 'fire').and.callThrough();

                testObservable.commit();

                expect(testObservable.events.fire).toHaveBeenCalledWith('local:change', data);
            });
        });
    });

    describe('extract', function() {
        var data, testObservable;

        beforeEach(function() {
            data = {'baz': 'bat'};
            testObservable = observableFactory.observe(data, EventSystemMock);
        });

        it('Will give a friendly data access method to an observable type\'s data field', function(){
            expect(observableFactory.extract(testObservable)).toEqual(data);
        });
    });
});