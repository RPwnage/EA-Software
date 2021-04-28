/**
 * Jasmine functional test
 */

'use strict';

describe('Home Discovery Tile Game Controller', function() {
    var $controller, $rootScope, PdpUrlFactory, OgdUrlFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(message){}
                }
            });

            $provide.factory('UtilFactory', function() {
                return {
                    isTouchEnabled: function(){
                        return false;
                    }
                };
            });

            $provide.factory('AppCommFactory', function() {
                return {
                    events: {
                        'fire': function(){},
                        'on': function(){}
                    }
                };
            });

            $provide.factory('PdpUrlFactory', function() {
                return {
                    goToPdp: function(){}
                };
            });

            $provide.factory('OgdUrlFactory', function() {
                return {
                    goToOgd: function(){}
                };
            });

            $provide.factory('ProductFactory', function(){
                return {};
            });

            $provide.factory('GamesDataFactory', function() {
                return {
                    getEntitlement: function(offerId) {
                        switch(offerId) {
                            case 'unowned_game':
                                return;
                            case 'owned_game':
                                return {
                                    entitlementId: 1010019301487
                                };
                        }
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _$rootScope_, _PdpUrlFactory_, _OgdUrlFactory_) {
            $controller = _$controller_;
            $rootScope = _$rootScope_;
            PdpUrlFactory = _PdpUrlFactory_;
            OgdUrlFactory = _OgdUrlFactory_;
    }));

    describe('Navigate to a game', function() {
        var $scope, controller;

        beforeEach(function() {
            $scope = $rootScope.$new();
            spyOn(PdpUrlFactory, 'goToPdp').and.callThrough();
            spyOn(OgdUrlFactory, 'goToOgd').and.callThrough();
            controller = $controller('OriginHomeDiscoveryTileGameCtrl', {
                $scope: $scope,
                $state: null,
                contextNamespace: null,
                customDescriptionFunction: null,
            });
        });

        it('will direct a user that owns the game to their owned game details page', function() {
            $scope.offerId = 'owned_game';
            $scope.model = {offerId: $scope.offerId};
            $scope.navigateToGame();
            expect(OgdUrlFactory.goToOgd).toHaveBeenCalled();
            expect(OgdUrlFactory.goToOgd).toHaveBeenCalledWith({
                offerId: 'owned_game'
            });
        });

        it('will direct a use that doesn\'t own the game yet to a product detail page', function() {
            $scope.offerId = 'unowned_game';
            $scope.model = {offerId: $scope.offerId};
            $scope.navigateToGame();
            expect(PdpUrlFactory.goToPdp).toHaveBeenCalled();
            expect(PdpUrlFactory.goToPdp).toHaveBeenCalledWith({
                offerId: 'unowned_game'
            });
        });
    });
});
