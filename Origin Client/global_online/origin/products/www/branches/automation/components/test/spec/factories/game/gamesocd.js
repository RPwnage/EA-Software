/**
 * Ocd adapter factory
 */

'use strict';

describe('The Origin Content Database (OCD) Service Adapter', function() {
    var gamesOcdFactory;

    beforeEach(function () {
        window.OriginComponents = {};
        window.Origin = {};
        window.Origin.games = {
                getOcdByPath: function() {}
        };
        window.Origin.locale = {
                locale: function() { return 'en_US'; },
                threeLetterCountryCode: function() { return 'USA'; }
        };
        module('origin-components');
        module(function($provide){
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){}
                };
            });

            $provide.factory('GamesDataFactory', function(){
                return {
                    events: {
                        'on': function(){},
                        'fire': function(){}
                    }
                };
            });
        });
    });

    var gameHubResponse = function () {
        var headers = ['Cache-Control: public, store, max-age=300, s-maxage=300', 'Content-Type: application/json;charset=UTF-8'].join("\r\n");

        return Promise.resolve({
            headers: headers,
            data: {
                "gamehub" : {
                    "components": {
                        "items": []
                    }
                }
            }
        });
    }

    beforeEach(inject(function (_GamesOcdFactory_) {
        gamesOcdFactory = _GamesOcdFactory_;
    }));


    describe('getOcdByOfferId - get a gamehub page from the jssdk usind EADP offer Id', function() {
        describe('getOcdByPath - get a gamehub page from the jssdk using ocdPath', function() {
            it('will get a gamehub page by ocd path and default locale', function(done) {
                spyOn(window.Origin.games, 'getOcdByPath').and.callFake(gameHubResponse);

                gamesOcdFactory.getOcdByPath('battlefield/battlefield-4/standard-edition')
                    .then(function(){
                        expect(window.Origin.games.getOcdByPath).toHaveBeenCalled();
                        expect(window.Origin.games.getOcdByPath).toHaveBeenCalledWith(
                            'en-us.usa',
                            'battlefield/battlefield-4/standard-edition'
                        );
                        done();
                    })
                    .catch(function(err){
                        fail(err);
                        done();
                    });
            });
            it('will get a gamehub page get a gamehub page from the jssdk using ocdPath and locale', function(done) {
                spyOn(window.Origin.games, 'getOcdByPath').and.callFake(gameHubResponse);

                gamesOcdFactory.getOcdByPath('battlefield/battlefield-4/standard-edition', 'en-gb.gbr')
                    .then(function(){
                        expect(window.Origin.games.getOcdByPath).toHaveBeenCalled();
                        expect(window.Origin.games.getOcdByPath).toHaveBeenCalledWith(
                            'en-gb.gbr',
                            'battlefield/battlefield-4/standard-edition'
                        );
                        done();
                    })
                    .catch(function(err){
                        fail(err);
                        done();
                    });
            });

            it('will get a gamehub page by master title only ocd path', function(done) {
                spyOn(window.Origin.games, 'getOcdByPath').and.callFake(gameHubResponse);

                gamesOcdFactory.getOcdByPath('battlefield/battlefield-4', 'en-gb.gbr')
                    .then(function(){
                        expect(window.Origin.games.getOcdByPath).toHaveBeenCalled();
                        expect(window.Origin.games.getOcdByPath).toHaveBeenCalledWith(
                            'en-gb.gbr',
                            'battlefield/battlefield-4'
                        );
                        done();
                    })
                    .catch(function(err){
                        fail(err);
                        done();
                    });
            });
        });
    });

});
