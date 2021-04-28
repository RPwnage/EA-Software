/**
 * Jasmine functional test
 */

'use strict';

describe('Ogd URL Factory', function() {
    var OgdUrlFactory, AppCommFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(message){}
                }
            });

            $provide.factory('AppCommFactory', function() {
                return {
                    'events': {
                        'fire': function(){},
                        'on': function(){}
                    }
                };
            });
        });
    });

    describe('Go to Owned Game Details', function(){
        beforeEach(inject(function(_OgdUrlFactory_, _AppCommFactory_) {
            OgdUrlFactory = _OgdUrlFactory_;
            AppCommFactory = _AppCommFactory_;
            spyOn(AppCommFactory.events, 'fire').and.callThrough();
        }));

        it('will fire an event telling the application to navigate to the ogd page', function(){
            OgdUrlFactory.goToOgd({
                offerId: 'owned_game'
            });
            expect(AppCommFactory.events.fire).toHaveBeenCalled();
            expect(AppCommFactory.events.fire).toHaveBeenCalledWith(
                'uiRouter:go', 'app.game_gamelibrary.ogd', {'offerid': 'owned_game'}
            );
        })
    });
});