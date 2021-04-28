/**
 * Jasmine functional test
 */

'use strict';

describe('Programmed Violator Factory', function() {
    var GameProgrammedViolatorFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('GamesDataFactory', function() {
                return {
                    getOcdByOfferId: function(offerId) {
                        switch(offerId) {
                            case 'fullypopulatedgamehub':
                                return Promise.resolve({
                                    'gamehub': {
                                        'components': {
                                            'origin-game-tile': {
                                                'items': [
                                                    {
                                                        'origin-game-violator': {
                                                            title: 'A game tile title',
                                                            iconcolor: 'white',
                                                            icon: 'timer',
                                                            priority: 'normal'
                                                        }
                                                    }, {
                                                        'origin-game-violator': {
                                                            title: 'A second game tile at a higher violator priority',
                                                            iconcolor: 'yellow',
                                                            icon: 'info',
                                                            priority: 'high'
                                                        }
                                                    }, {
                                                        'not-the-right-directive': {
                                                            wrongprop: 'wrong'
                                                        }
                                                    }
                                                ]
                                            },
                                            'origin-gamelibrary-ogd-header': {
                                                'items': [
                                                    {
                                                        'origin-gamelibrary-important-information': {
                                                            title: 'An OGD important info entry',
                                                            icon: 'warning',
                                                            iconcolor: 'red',
                                                            dismissible: 'true',
                                                            priority: 'low'
                                                        }
                                                    }
                                                ]
                                            }
                                        }
                                    }
                                });
                            case 'emptygamehub':
                                return Promise.resolve({
                                    'gamehub': {
                                        'components': {
                                        }
                                    }
                                });
                        }
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_GameProgrammedViolatorFactory_) {
        GameProgrammedViolatorFactory = _GameProgrammedViolatorFactory_;
    }));

    describe('Violator request for a populated gamehub', function(){
        it('should return a programmed violator model in the natural order it was provided from the service', function(done) {
            GameProgrammedViolatorFactory.getContent('fullypopulatedgamehub', 'origin-game-tile', 'origin-game-violator')
            .then(function(data) {
                expect(data.length).toBe(2);
                expect(data[0].title).toBe('A game tile title');
                expect(data[1].title).toBe('A second game tile at a higher violator priority');
                done();
            })
            .catch(function(err){
                fail(err);
                done();
            });
        });

        it('should still provide an array result when there is only onedirective programmed', function(done) {
            GameProgrammedViolatorFactory.getContent('fullypopulatedgamehub', 'origin-gamelibrary-ogd-header', 'origin-gamelibrary-important-information')
            .then(function(data) {
                expect(data.length).toBe(1);
                expect(data[0].title).toBe('An OGD important info entry');
                done();
            })
            .catch(function(err){
                fail(err);
                done();
            });
        });

        it('should still provide an array result when there is no data found', function(done) {
            GameProgrammedViolatorFactory.getContent('emptygamehub', 'origin-gamelibrary-ogd-header', 'origin-gamelibrary-important-information')
            .then(function(data) {
                expect(data.length).toBe(0);
                done();
            })
            .catch(function(err){
                fail(err);
                done();
            });
        });
    });
});