/**
 * Jasmine functional test
 */

'use strict';

describe('Observer Factory', function() {
    var observerFactory, observableFactory, EventSystemMock;

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
            this.on = function() {
                return {
                    detach: function() {}
                };
            };
            this.off = function(){};
            this.fire = function(){};
        };
    });

    beforeEach(inject(function(_ObserverFactory_, _ObservableFactory_) {
        observerFactory = _ObserverFactory_;
        observableFactory = _ObservableFactory_;
    }));

    describe('create', function() {
        it('Will create a new observer object', function(){
            var data = {'foo': 'bar'};
            var observable = observableFactory.observe(data, EventSystemMock);

            var observer1 = observerFactory.create(observable);
            var observer2 = observerFactory.create(observable);

            expect(observer1).not.toBe(observer2);
        });

        describe('Observer response object filters', function() {
            var testData, testObservable, testObserver, mockScope;

            beforeEach(function() {
                testData = [{'foo': 'bar'}, {'baz': 'bat'}];
                testObservable = observableFactory.observe(testData, EventSystemMock);
                testObserver = observerFactory.create(testObservable);
                mockScope = {
                    $on: function() {},
                    $off: function() {},
                    $$phase: false,
                    $digest: function() {}
                };
            });

            /**
             * The filtering behavior tests are proven in ObjectHelperFactory
             * This is a happy path test with known good inputs for reference
             */
            it('Will add a custom filter function to the observer\'s property delta lookup', function() {
                spyOn(testObserver.filters, 'push').and.callThrough();
                testObserver.addFilter(_.head);

                expect(testObserver.filters.push).toHaveBeenCalled();
            });

            it('Will filter by matching keys and values on each change cycle', function() {
                spyOn(testObserver.filters, 'push').and.callThrough();

                var filterConditions = {'foo': 'bar'};
                testObserver.filterBy(filterConditions);

                expect(testObserver.filters.push).toHaveBeenCalled();
            });

            it('will filter by a single property on a change cycle', function() {
                spyOn(testObserver.filters, 'push').and.callThrough();

                testObserver.getProperty('baz');

                expect(testObserver.filters.push).toHaveBeenCalled();
            });

            it('will filter by multiple properties on a change cycle', function() {
                spyOn(testObserver.filters, 'push').and.callThrough();

                testObserver.pick(['foo', 'baz']);

                expect(testObserver.filters.push).toHaveBeenCalled();
            });

            it('will default to a value if the value is null or undefined', function() {
                spyOn(testObserver.filters, 'push').and.callThrough();

                testObserver.defaultTo('empty set');

                expect(testObserver.filters.push).toHaveBeenCalled();
            });
            /* End of filtering pass*/

            it('will attach an observer to AngularJS $scope object property and register a callback in the observable', function() {
                spyOn(testObservable.events, 'on').and.callThrough();

                testObserver.attachToScope(mockScope, 'myModel');

                expect(testObservable.events.on).toHaveBeenCalled();
            });

            it('will throw an exception if attach to scope is passed an invalid type', function(){
                expect(function(){
                    testObserver.attachToScope(mockScope, {});
                }).toThrow();
            });
        });

        describe('After attaching an observer to scope', function(){
            var testData, testObservable, testObserver, mockScope, EventPassingMock;

            beforeEach(function() {
                EventPassingMock = function() {
                    this.on = function(name, callback) {
                        this.callback = callback;
                        return {
                            detach: function() {
                                this.callback = undefined;
                            }
                        };
                    };
                    this.off = function(){
                        this.callback = undefined;
                    };
                    this.fire = function(name, data){
                        if(_.isFunction(this.callback)) {
                            this.callback.apply(undefined, [data]);
                        }
                    };
                };

                testData = {'foo': 'bar'};
                testObservable = observableFactory.observe(testData, EventPassingMock);
                testObserver = observerFactory.create(testObservable);

                mockScope = {
                    $on: function() {},
                    $off: function() {},
                    $$phase: false,
                    $digest: function() {}
                };
            });

            it('Will update scope using $digest on commit() to an observable', function() {
                testObserver.attachToScope(mockScope, 'anotherModel');

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).toHaveBeenCalled();
            });

            it('Will call a custom digester on commit() to an observable', function() {
                spyOn(observerFactory, 'noDigest').and.callThrough();
                testObserver.attachToScope(mockScope, 'anotherModel', observerFactory.noDigest);

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).not.toHaveBeenCalled();
                expect(observerFactory.noDigest).toHaveBeenCalled();
            });

            it('Will detach the event from scope', function() {
                var handle = testObserver.attachToScope(mockScope, 'anotherModel');

                spyOn(mockScope, '$digest').and.callThrough();
                expect(testObservable.callback).not.toBe(null);

                handle.detach();

                testObservable.commit();

                expect(mockScope.$digest).not.toHaveBeenCalled();
                expect(testObservable.callback).toBe(null);
            });

            it('Will optionally filter the scope data on a missing property - scope digester still called', function() {
                //pipeline observable.data->filterset(recursive)->set(filtered data)->digest(setter)
                testObserver
                    .getProperty('notAProperty')
                    .attachToScope(mockScope, 'filteredModel1');

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).toHaveBeenCalled();
                expect(mockScope.filteredModel1).toEqual(undefined);
            });

            it('Will optionally filter the scope data on a matching property - scope digester still called', function() {
                testObserver
                    .getProperty('foo')
                    .attachToScope(mockScope, 'filteredModel2');

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).toHaveBeenCalled();
                expect(mockScope.filteredModel2).toEqual('bar');
            });

            it('Will default to a value if data is empty', function() {
                testObservable.data = undefined;

                testObserver
                    .defaultTo('shirley')
                    .attachToScope(mockScope, 'filteredModel3');

                expect(mockScope.filteredModel3).toEqual('shirley');

                testObservable.data = 'bob';
                testObservable.commit();

                expect(mockScope.filteredModel3).toEqual('bob');
            });

            it('will automatically unwrap promises if the data filter returns a promise', function(done) {
                var data = {foo:'bar'};

                var getModel = function() {
                    return Promise.resolve(data);
                };

                testObserver
                    .addFilter(getModel)
                    .attachToScope(mockScope, 'filteredModel4');

                //There's no direct access to the unwrapper logic, so do a short, soft-realtime wait for the mock promise to resolve
                setTimeout(function(){
                    expect(mockScope.filteredModel4.foo).toEqual('bar');
                    done();
                }, 1);
            });

            it('will throw exceptions if invalid input is provided to the filters', function() {
                expect(function() {
                    testObservable.addFilter('');
                }).toThrow();

                expect(function() {
                    testObservable.filterBy('');
                }).toThrow();

                expect(function() {
                    testObservable.getProperty(function(){});
                }).toThrow();
            });
        });

        describe('onUpdate - using observers with callback functions', function(){
            var testData, testFunctions, testObservable, testObserver, mockScope, EventPassingMock;

            beforeEach(function() {
                EventPassingMock = function() {
                    this.on = function(name, callback) {
                        this.callback = callback;
                        return {
                            detach: function() {
                                this.callback = undefined;
                            }
                        };
                    };
                    this.off = function(){
                        this.callback = undefined;
                    };
                    this.fire = function(name, data){
                        if(_.isFunction(this.callback)) {
                            this.callback.apply(undefined, [data]);
                        }
                    };
                };

                testData = {'foo': 'bar'};
                testObservable = observableFactory.observe(testData, EventPassingMock);
                testObserver = observerFactory.create(testObservable);
                testFunctions = {
                    mockCallback: function() {}
                };

                mockScope = {
                    $on: function() {},
                    $off: function() {},
                    $$phase: false,
                    $digest: function() {}
                };
            });

            it('Will update scope using $digest on commit() to an observable', function() {
                spyOn(testFunctions, 'mockCallback').and.callThrough();

                testObserver.onUpdate(mockScope, testFunctions.mockCallback);

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).toHaveBeenCalled();
                expect(testFunctions.mockCallback).toHaveBeenCalled();
            });

            it('Will call a custom digester on commit() to an observable', function() {
                spyOn(testFunctions, 'mockCallback').and.callThrough();

                testObserver.onUpdate(mockScope, testFunctions.mockCallback, observerFactory.noDigest);

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).not.toHaveBeenCalled();
                expect(testFunctions.mockCallback).toHaveBeenCalled();
            });

            it('Will detach the event from scope', function() {
                var callbackSpy = jasmine.createSpy('callbackSpy');
                var handle = testObserver.onUpdate(mockScope, callbackSpy);

                spyOn(mockScope, '$digest').and.callThrough();
                expect(testObservable.callback).not.toBe(null);
                expect(callbackSpy.calls.count()).toBe(1);

                handle.detach();

                testObservable.commit();

                expect(mockScope.$digest).not.toHaveBeenCalled();
                expect(callbackSpy.calls.count()).toBe(1);
                expect(testObservable.callback).toBe(null);
            });

            it('Will optionally filter the scope data on a missing property - scope digester still called', function() {
                spyOn(testFunctions, 'mockCallback').and.callThrough();

                //pipeline observable.data->filterset(recursive)->set(filtered data)->digest(setter)
                testObserver
                    .getProperty('notAProperty')
                    .onUpdate(mockScope, testFunctions.mockCallback);

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).toHaveBeenCalled();
                expect(testFunctions.mockCallback).toHaveBeenCalledWith(undefined);
            });

            it('Will optionally filter the scope data on a matching property - scope digester still called', function() {
                spyOn(testFunctions, 'mockCallback').and.callThrough();

                testObserver
                    .getProperty('foo')
                    .onUpdate(mockScope, testFunctions.mockCallback);

                spyOn(mockScope, '$digest').and.callThrough();

                testObservable.commit();

                expect(mockScope.$digest).toHaveBeenCalled();
                expect(testFunctions.mockCallback).toHaveBeenCalledWith('bar');
            });

            it('Will default to a value if data is empty', function() {
                spyOn(testFunctions, 'mockCallback').and.callThrough();

                testObservable.data = undefined;

                testObserver
                    .defaultTo('shirley')
                    .onUpdate(mockScope, testFunctions.mockCallback);

                expect(testFunctions.mockCallback).toHaveBeenCalledWith('shirley');

                testObservable.data = 'bob';
                testObservable.commit();

                expect(testFunctions.mockCallback).toHaveBeenCalledWith('bob');
            });

            it('will pass through promises to the callback for post processing', function() {
                var data = {foo:'bar'};

                var getModel = function() {
                    return Promise.resolve(data);
                };

                spyOn(testFunctions, 'mockCallback').and.callThrough();

                testObserver
                    .addFilter(getModel)
                    .onUpdate(mockScope, testFunctions.mockCallback);

                expect(testFunctions.mockCallback).toHaveBeenCalledWith(getModel());
            });

            it('will allow the user to omit scope in case of use in contexts that are not scope aware', function() {
                spyOn(testFunctions, 'mockCallback').and.callThrough();
                spyOn(mockScope, '$digest').and.callThrough();
                spyOn(mockScope, '$on').and.callThrough();

                testObserver.onUpdate(undefined, testFunctions.mockCallback);

                testObservable.commit();

                expect(mockScope.$digest).not.toHaveBeenCalled();
                expect(mockScope.$on).not.toHaveBeenCalled();
                expect(testFunctions.mockCallback).toHaveBeenCalled();
            });
        });
    });
});