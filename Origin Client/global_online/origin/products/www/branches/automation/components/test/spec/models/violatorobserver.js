/**
 * Jasmine functional test
 */

'use strict';

describe('Violator observer factory', function() {
    var violatorObserverFactory,
        violatorFactory,
        observerFactory,
        gametileViolatorFactory,
        ogdHeaderViolatorFactory,
        mockScope;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            'utils': {
                'Communicator': function() {}
            }
        };
        angular.mock.module('origin-components');
        module(function($provide){
            function eventsMock() {
                return {
                    events: {
                        on: function(){}
                    }
                };
            }

            $provide.factory('GameViolatorFactory', eventsMock);
            $provide.factory('AuthFactory', eventsMock);
            $provide.factory('GamesClientFactory', eventsMock);
            $provide.factory('OgdHeaderViolatorFactory', function(){
                return {
                    getInlineMessages: function() {},
                    getDialogMessages: function() {},
                    getReadMoreLinkHtml: function() {}
                };
            });
            $provide.factory('GametileViolatorFactory', function(){
                return {
                    getInlineMessages: function() {},
                    getDialogMessages: function() {},
                    getReadMoreLinkHtml: function() {}
                };
            });
            $provide.factory('GameViolatorFactory',  function() {
                return {
                    events: {
                        on: function(){}
                    },
                    getViolators: function() {},
                    isValidViolatorData: function() {}
                };
            });
            $provide.factory('GamesDataFactory', function() {
                return {
                    events: {
                        on: function(){}
                    },
                    getCatalogInfo: function() {
                        return Promise.resolve({
                            'OFB-EAST:12345': {
                                offerId: 'OFB-EAST:12345',
                                i18n: {
                                    displayName: 'game foo'
                                }
                            }
                        });
                    }
                };
            });
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    error: function() {}
                };
            });
            $provide.factory('ObservableFactory', function() {
                return {
                    observe: function() {
                        return {
                            commit: function() {}
                        };
                    }
                };
            });
            $provide.factory('ObserverFactory', function() {
                return {
                    create: function() {
                        return {
                            addFilter: function() {
                                return this;
                            },
                            onUpdate: function() {
                                return this;
                            }
                        };
                    },
                    noDigest: function() {}
                };
            });
        });

        mockScope = {
            $on: function() {},
            $destroy: function() {}
        };
    });

    beforeEach(inject(function(_ViolatorObserverFactory_, _GameViolatorFactory_,  _ObserverFactory_, _GametileViolatorFactory_, _OgdHeaderViolatorFactory_) {
        violatorObserverFactory = _ViolatorObserverFactory_;
        violatorFactory = _GameViolatorFactory_;
        observerFactory = _ObserverFactory_;
        gametileViolatorFactory = _GametileViolatorFactory_;
        ogdHeaderViolatorFactory = _OgdHeaderViolatorFactory_;
    }));

    describe('getObserver', function() {
        it('will provide an observer handle', function() {
            var offerId = 'OFB-EAST:12345',
                viewContext = 'importantinfo',
                inlineCount = 2,
                messages = {},
                callbackFunc = function(){};

            var observer = violatorObserverFactory.getObserver(offerId, viewContext, inlineCount, messages, mockScope, callbackFunc);

            expect(typeof(observer)).toEqual('object');
        });
    });

    describe('getModel - game tiles', function() {
        it('Will provide a view model for use in the AngularJs controller', function(done) {
            var offerId = 'OFB-EAST:12345',
                viewContext = 'gametile',
                inlineCount = 1,
                messages,
                violatorData = [{
                    'violatortype': 'automatic',
                    'priority': 'critical',
                    'offerid': 'OFB-EAST:12345',
                }, {
                    'violatortype': 'programed',
                    'priority': 'normal',
                    'offerid': 'OFB-EAST:12345',
                }];

            spyOn(violatorFactory, 'getViolators').and.returnValue(violatorData);
            spyOn(violatorFactory, 'isValidViolatorData').and.returnValue(true);
            spyOn(gametileViolatorFactory, 'getInlineMessages').and.callThrough();

            var model = violatorObserverFactory.getModel(offerId, viewContext, inlineCount, messages);

            model.then(function() {
                    expect(violatorFactory.isValidViolatorData).toHaveBeenCalledWith(violatorData);
                    expect(gametileViolatorFactory.getInlineMessages).toHaveBeenCalledWith(violatorData, offerId, inlineCount);
                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });
    });

    describe('getModel - ogd header tiles', function() {
        it('Will provide a view model for use in the AngularJs controller', function(done) {
            var offerId = 'OFB-EAST:12345',
                viewContext = 'importantinfo',
                inlineCount = 1,
                messages = {
                    gamemessages: function() {return 'Play this game';},
                    viewmoremessagesctaCallback: function() {return 'view more';},
                    closemessagescta: 'close'
                },
                violatorData = [{
                    'violatortype': 'automatic',
                    'priority': 'critical',
                    'offerid': 'OFB-EAST:12345',
                }, {
                    'violatortype': 'programed',
                    'priority': 'normal',
                    'offerid': 'OFB-EAST:12345',
                }];

            spyOn(violatorFactory, 'getViolators').and.returnValue(violatorData);
            spyOn(violatorFactory, 'isValidViolatorData').and.returnValue(true);
            spyOn(ogdHeaderViolatorFactory, 'getInlineMessages').and.callThrough();
            spyOn(ogdHeaderViolatorFactory, 'getDialogMessages').and.callThrough();
            spyOn(ogdHeaderViolatorFactory, 'getReadMoreLinkHtml').and.callThrough();

            var model = violatorObserverFactory.getModel(offerId, viewContext, inlineCount, messages);

            model.then(function() {
                    expect(violatorFactory.isValidViolatorData).toHaveBeenCalledWith(violatorData);
                    expect(ogdHeaderViolatorFactory.getInlineMessages).toHaveBeenCalledWith(violatorData, offerId, inlineCount);
                    expect(ogdHeaderViolatorFactory.getDialogMessages).toHaveBeenCalledWith(violatorData, offerId, messages.gamemessages(), messages.closemessagescta);
                    expect(ogdHeaderViolatorFactory.getReadMoreLinkHtml).toHaveBeenCalledWith(2, 1, messages.viewmoremessagesctaCallback);
                    done();
                })
                .catch(function(err) {
                    this.fail(Error(err));
                    done();
                });
        });
    });
});