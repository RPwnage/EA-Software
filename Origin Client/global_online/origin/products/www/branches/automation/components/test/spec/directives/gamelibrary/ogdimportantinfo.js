/**
 * Jasmine functional test
 */

'use strict';

describe('Important Information Tile', function() {
    var $controller, UtilFactory, GameViolatorFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        window.Origin = {
            client: {
                onlineStatus: {
                    goOnline: function() {}
                },
                isEmbeddedBrowser: function() { return true; }
            },
            locale: {
                languageCode: function() {
                    return '';
                },
                countryCode: function() {
                    return '';
                },
                threeLetterCountryCode: function() {
                    return '';
                },
                locale: function() {
                    return '';
                }
            }
        };

        angular.mock.module('origin-components');
        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function() {
                return {
                    log: function() {}
                };
            });

            $provide.factory('GameViolatorFactory', function() {
                return {
                    dismiss: function() {},
                    isWithinViewableDateRange: function() {}
                };
            });
        });
    });

    beforeEach(inject(function(_$controller_, _UtilFactory_, _GameViolatorFactory_) {
        $controller = _$controller_;
        UtilFactory = _UtilFactory_;
        GameViolatorFactory = _GameViolatorFactory_;
    }));

    describe('$scope', function() {
        var $scope, $event, $state, Md5Factory, controller, PurchaseFactory, GamesDataFactory, AuthFactory, DialogFactory;

        beforeEach(function() {
            $scope = {'$digest': function(){}, '$evalAsync': function(){}};
            $event = { stopPropagation: function() {} };

            Md5Factory = {
                md5: function() {
                    return "md5hash";
                }
            };

            PurchaseFactory = {
                renewSubscription: function() {},
                entitleVaultGameFromLesserBaseGame: function() {}
            };

            GamesDataFactory = {
                buyNow: function() {},
                hideGame: function() {},
                vaultRemove: function() {}
            };

            AuthFactory = {
                isClientOnline: function() { return true; }
            };

            DialogFactory = {
                openAlert: function() {},
                close: function() {}
            };

            $state = {
                go: function() {}
            };

            controller = $controller('OriginGamelibraryOgdImportantInfoCtrl', {
                $scope: $scope,
                $state: $state,
                UtilFactory: UtilFactory,
                Md5Factory: Md5Factory,
                PurchaseFactory: PurchaseFactory,
                GamesDataFactory: GamesDataFactory,
                AuthFactory: AuthFactory,
                DialogFactory: DialogFactory
            });
        });

        it('Allows for dismissibility if the dismissibile flag is set to "true"', function() {
            $scope.dismissible = 'true';
            expect($scope.isDismissible()).toEqual(true);
        });

        it('Prohibits dismissibility if flag is set to "false"', function(){
            $scope.dismissible = 'false';
            expect($scope.isDismissible()).toEqual(false);
        });

        it('Prohibits dismissibility if flag is set to a value not in the value map', function() {
            $scope.dismissible = 'lizard';
            expect($scope.isDismissible()).toEqual(false);
        });

        it('Will identify an icon class if a valid icon enumeration is provided', function() {
            $scope.icon = 'timer';
            expect($scope.getIconClass()).toEqual('otkicon-timer');
            expect($scope.hasIcon()).toEqual(true);
        });

        it('Will identify an icon class with a color if a valid icon and iconcolor enumeration is provided', function() {
            $scope.icon = 'warning';
            $scope.iconcolor = 'red';
            expect($scope.getIconClass()).toEqual('otkicon-warning red');
            expect($scope.hasIcon()).toEqual(true);
        });

        it('Will deny invalid icons', function() {
            $scope.icon = '290ei209ie029ie90';
            expect($scope.getIconClass()).toEqual('');
            expect($scope.hasIcon()).toEqual(false);
        });

        it('will provide hyperlink functionality to remove a vault game', function(done) {
            spyOn(GamesDataFactory, 'vaultRemove').and.returnValue(Promise.resolve());
            spyOn($event, 'stopPropagation');

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.removeVaultGame($event)).toBe(false);

            setTimeout(function() {
                expect(GamesDataFactory.vaultRemove).toHaveBeenCalledWith($scope.offerid);
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for hiding a game', function(done) {
            spyOn(GamesDataFactory, 'hideGame');
            spyOn($event, 'stopPropagation');

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.hideGame($event)).toBe(false);

            setTimeout(function() {
                expect(GamesDataFactory.hideGame).toHaveBeenCalledWith($scope.offerid);
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for renewing a subscription', function(done) {
            spyOn(PurchaseFactory, 'renewSubscription');
            spyOn($event, 'stopPropagation');

            expect($scope.renewSubscription($event)).toBe(false);

            setTimeout(function() {
                expect(PurchaseFactory.renewSubscription).toHaveBeenCalled();
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for buying a game', function(done) {
            spyOn(GamesDataFactory, 'buyNow');
            spyOn($event, 'stopPropagation');

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.goToPdp($event)).toBe(false);

            setTimeout(function() {
                expect(GamesDataFactory.buyNow).toHaveBeenCalledWith($scope.offerid);
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for buying a game', function(done) {
            spyOn(GamesDataFactory, 'buyNow');
            spyOn($event, 'stopPropagation');

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.goToPdp($event)).toBe(false);

            setTimeout(function() {
                expect(GamesDataFactory.buyNow).toHaveBeenCalledWith($scope.offerid);
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for going back online', function(done) {
            spyOn(window.Origin.client.onlineStatus, 'goOnline');
            spyOn(DialogFactory, 'close');
            spyOn($event, 'stopPropagation');

            expect($scope.goOnline($event)).toBe(false);

            setTimeout(function() {
                expect(window.Origin.client.onlineStatus.goOnline).toHaveBeenCalled();
                expect(DialogFactory.close).toHaveBeenCalledWith('ogd-violator-read-more-dialog');
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will nag the user to go back online if the client is offline', function(done) {
            spyOn(GamesDataFactory, 'buyNow');
            spyOn($event, 'stopPropagation');
            spyOn(DialogFactory, 'openAlert');
            spyOn(DialogFactory, 'close');
            spyOn(AuthFactory, 'isClientOnline').and.returnValue(false);
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(true);

            $scope.offerid = 'OFBEAST:12423';
            $scope.offlinemodaltitle = 'Oops, you\'re offline!';
            $scope.offlinemodaldescription = 'You\'ll have to go online to perform this action.';
            $scope.offlinemodaldismissbutton = 'OK';

            expect($scope.goToPdp($event)).toBe(false);

            setTimeout(function() {
                expect(GamesDataFactory.buyNow).not.toHaveBeenCalled();
                expect($event.stopPropagation).toHaveBeenCalled();
                expect(AuthFactory.isClientOnline).toHaveBeenCalled();
                expect(window.Origin.client.isEmbeddedBrowser).toHaveBeenCalled();
                expect(DialogFactory.close).toHaveBeenCalledWith('ogd-violator-read-more-dialog');
                expect(DialogFactory.openAlert).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will not check non-embedded browser clients for offline', function(done) {
            spyOn(GamesDataFactory, 'buyNow');
            spyOn($event, 'stopPropagation');
            spyOn(DialogFactory, 'openAlert');
            spyOn(AuthFactory, 'isClientOnline').and.returnValue(false);
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.goToPdp($event)).toBe(false);

            setTimeout(function() {
                expect(GamesDataFactory.buyNow).toHaveBeenCalledWith($scope.offerid);
                expect($event.stopPropagation).toHaveBeenCalled();
                expect(AuthFactory.isClientOnline).not.toHaveBeenCalled();
                expect(window.Origin.client.isEmbeddedBrowser).toHaveBeenCalled();
                expect(DialogFactory.openAlert).not.toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for upgrading a vault enabled game', function(done) {
            spyOn(PurchaseFactory, 'entitleVaultGameFromLesserBaseGame').and.returnValue(Promise.resolve({}));
            spyOn($event, 'stopPropagation');

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.upgradeVaultGame($event)).toBe(false);

            setTimeout(function() {
                expect(PurchaseFactory.entitleVaultGameFromLesserBaseGame).toHaveBeenCalledWith($scope.offerid);
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });

        it('will provide hyperlink functionality for dismissing a violator', function(done) {
            spyOn(GameViolatorFactory, 'dismiss').and.returnValue(Promise.resolve({}));
            spyOn($event, 'stopPropagation');

            $scope.offerid = 'OFBEAST:12423';
            expect($scope.dismiss($event)).toBe(false);

            setTimeout(function() {
                expect(GameViolatorFactory.dismiss).toHaveBeenCalled();
                expect($event.stopPropagation).toHaveBeenCalled();
                done();
            }, 32);
        });
    });
});
