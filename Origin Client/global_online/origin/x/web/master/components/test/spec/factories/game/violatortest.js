/**
 * Jasmine functional test
 */

'use strict';

var Origin = {
    'utils': {
        os: function() {
            return 'PCWIN';
        }
    }
};

describe('Automatic Game Violator Factory', function() {
    var GameAutomaticViolatorTestsFactory;

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        module(function($provide) {
            function getPastDate(){
                var pastDate = new Date();
                pastDate.setMinutes(pastDate.getMinutes()-20);
                return pastDate;
            }

            function getFutureDate(){
                var futureDate = new Date();
                futureDate.setMinutes(futureDate.getMinutes()+20);
                return futureDate;
            }

            $provide.factory('GamesDataFactory', function() {
                var expiredNewDlcDate = new Date();
                expiredNewDlcDate.setHours(expiredNewDlcDate.getHours()-(8 * 24));
                var expiredNewDlcExpansionDate = new Date();
                expiredNewDlcExpansionDate.setHours(expiredNewDlcExpansionDate.getHours()-(29 * 24));
                var expiredReadyDlcDate = new Date();
                expiredReadyDlcDate.setHours(expiredReadyDlcDate.getHours()-(8 * 24));
                var expiredInstalledDlcDate = new Date();
                expiredInstalledDlcDate.setHours(expiredInstalledDlcDate.getHours()-(8 * 24));

                return{
                    getClientGame: function(offerId) {
                        switch (offerId) {
                            case 'preloadavailable':
                                return {
                                    releaseStatus: 'PRELOAD'
                                };
                            case 'updateavailable':
                                return {
                                    updateAvailable: true
                                };
                            case 'newdlcreadyforinstallsubcontent':
                                return {
                                    downloadable: true,
                                    installable: true
                                };
                            case 'newdlcreadyforinstallsubcontent7days':
                                return {
                                    downloadable: true,
                                    installable: true
                                };
                            case 'newdlcinstalledsubcontent':
                                return{
                                    installed: true,
                                    initialInstalledTimestamp: getFutureDate()
                                };
                            case 'newdlcinstalledsubcontent7days':
                                return {
                                    installed: true,
                                    initialInstalledTimestamp: expiredInstalledDlcDate
                                };
                            default:
                                return {
                                    downloading: false
                                };
                        }
                    },
                    getEntitlement: function(offerId) {
                        switch (offerId) {
                            case 'gameexpiresfuture':
                                return {
                                    terminationDate: getFutureDate()
                                };
                            case 'gameexpirespast':
                                return {
                                    terminationDate: getPastDate()
                                };
                            case 'gameexpiresfuturetrial':
                                return {
                                    terminationDate: getFutureDate()
                                };
                            case 'trialnotactivated':
                                return {
                                    terminationDate: null
                                };
                            case 'trialnotexpired':
                                return {
                                    terminationDate: getFutureDate()
                                };
                            case 'trialexpired':
                                return {
                                    terminationDate: getPastDate()
                                };
                            case 'newdlcreadyforinstallsubcontent':
                                return {
                                    grantDate: getPastDate()
                                };
                            case 'newdlcreadyforinstallsubcontent7days':
                                return {
                                    grantDate: expiredReadyDlcDate
                                };
                        }

                        return {};
                    },
                    getGameUsageInfo: function(offerId) {
                        switch(offerId) {
                            case 'newdlcinstalled':
                                return new Promise(function(resolve){
                                    resolve({
                                        //currently resolves to milliseconds since epoch
                                        lastPlayedTime: Number(getPastDate().getTime())
                                    });
                                });
                            case 'newdlcinstalled7days':
                                return new Promise(function(resolve){
                                    resolve({
                                        //currently resolves to milliseconds since epoch
                                        lastPlayedTime: Number(getPastDate().getTime())
                                    });
                                });
                            default:
                                return new Promise(function(resolve){resolve();});
                        }
                    },
                    getCatalogInfo: function(offerIds) {
                        var offerId = offerIds.shift();

                        switch (offerId) {
                            case 'preloadonfuture':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'preloadonfuture': {
                                            'downloadable' : true,
                                            'platforms': {
                                                'PCWIN': {
                                                    'downloadStartDate': getFutureDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'preloadonfuturedemobetatrial':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'preloadonfuturedemobetatrial': {
                                            'downloadable' : true,
                                            'beta': true,
                                            'trial': true,
                                            'demo': true,
                                            'platforms': {
                                                'PCWIN': {
                                                    'downloadStartDate': getFutureDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'preloadonpast':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'preloadonpast': {
                                            'downloadable' : true,
                                            'platforms': {
                                                'PCWIN': {
                                                    'downloadStartDate': getPastDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'releasesonfuture':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'releasesonfuture': {
                                            'platforms': {
                                                'PCWIN': {
                                                    'downloadStartDate': getFutureDate(),
                                                    'releaseDate': getFutureDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'releasesonpast':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'releasesonpast': {
                                            'platforms': {
                                                'PCWIN': {
                                                    'downloadStartDate': getPastDate(),
                                                    'releaseDate': getPastDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'releasesonfuturenopreload':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'releasesonfuturenopreload': {
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': getFutureDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'releasesonfuturenopreload':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'releasesonfuturenopreload': {
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': getFutureDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'gameexpiresfuture':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'gameexpiresfuture': {
                                            offerId: 'gameexpiresfuture'
                                        }
                                    });
                                });
                            case 'gameexpiresfuturetrial':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'gameexpiresfuturetrial': {
                                            trial: true,
                                            offerId: 'gameexpiresfuturetrial'
                                        }
                                    });
                                });
                            case 'trialnotactivated':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'trialnotactivated': {
                                            trial: true,
                                            offerId: 'trialnotactivated'
                                        }
                                    });
                                });
                            case 'trialnotexpired':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'trialnotexpired': {
                                            trial: true,
                                            offerId: 'trialnotexpired'
                                        }
                                    });
                                });
                            case 'trialexpired':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'trialexpired': {
                                            trial: true,
                                            offerId: 'trialexpired'
                                        }
                                    });
                                });
                            case 'newdlcavailable':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcavailable': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            case 'newdlcavailablepast7days':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcavailablepast7days': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            case 'newdlcexpansionavailable':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcexpansionavailable': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            case 'newdlcexpansionavailablepast28days':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcexpansionavailablepast28days': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            case 'newdlcreadyforinstall':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcreadyforinstall': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            case 'newdlcreadyforinstall7days':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcreadyforinstall': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            case 'newdlcinstalled':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcinstalled': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                           case 'newdlcinstalled7days':
                                return new Promise(function(resolve) {
                                    resolve({
                                        'newdlcinstalled': {
                                            'originDisplayType': 'Full Game'
                                        }
                                    });
                                });
                            default:
                                return new Promise(function(resolve){resolve();});
                        }
                    },
                    retrieveExtraContentCatalogInfo: function (parentOfferId, filter) {
                        switch(parentOfferId) {
                            case 'newdlcavailable':
                                return new Promise(function(resolve){
                                    resolve({
                                        'offer_1': {
                                            'downloadable': true,
                                            'purchasable': true,
                                            'originDisplayType': 'Addon',
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': getPastDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'newdlcavailablepast7days':
                                return new Promise(function(resolve){
                                    resolve({
                                        'offer_1': {
                                            'downloadable': true,
                                            'purchasable': true,
                                            'originDisplayType': 'Addon',
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': expiredNewDlcDate
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'newdlcexpansionavailable':
                                return new Promise(function(resolve){
                                    resolve({
                                        'offer_1': {
                                            'downloadable': true,
                                            'purchasable': true,
                                            'originDisplayType': 'Expansion',
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': getPastDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'newdlcexpansionavailablepast28days':
                                return new Promise(function(resolve){
                                    resolve({
                                        'offer_1': {
                                            'downloadable': true,
                                            'purchasable': true,
                                            'originDisplayType': 'Expansion',
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': expiredNewDlcExpansionDate
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'newdlcreadyforinstall':
                                return new Promise(function(resolve){
                                    resolve({
                                        'offer_1': {
                                            'downloadable': true,
                                            'originDisplayType': 'Expansion',
                                            'offerId': 'newdlcreadyforinstallsubcontent',
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': getPastDate()
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'newdlcreadyforinstall7days':
                                return new Promise(function(resolve){
                                    resolve({
                                        'offer_1': {
                                            'downloadable': true,
                                            'originDisplayType': 'Expansion',
                                            'offerId': 'newdlcreadyforinstallsubcontent7days',
                                            'platforms': {
                                                'PCWIN': {
                                                    'releaseDate': expiredReadyDlcDate
                                                }
                                            }
                                        }
                                    });
                                });
                            case 'newdlcinstalled':
                                return new Promise(function(resolve){
                                    resolve({
                                        'newdlcinstalledsubcontent': {
                                            'downloadable': true,
                                            'originDisplayType': 'Expansion',
                                            'offerId': 'newdlcinstalledsubcontent'
                                        }
                                    });
                                });
                            case 'newdlcinstalled7days':
                                return new Promise(function(resolve){
                                    resolve({
                                        'newdlcinstalledsubcontent': {
                                            'downloadable': true,
                                            'originDisplayType': 'Expansion',
                                            'offerId': 'newdlcinstalledsubcontent7days'
                                        }
                                    });
                                });
                        }
                    }
                };
            });
        });
    });

    beforeEach(inject(function(_GameAutomaticViolatorTestsFactory_) {
        GameAutomaticViolatorTestsFactory = _GameAutomaticViolatorTestsFactory_;
    }));

    describe('preloadon before release date', function() {
        it('will return true if the preload date is in the future', function(done) {
            GameAutomaticViolatorTestsFactory
                .preloadOn('preloadonfuture')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('preloadon before release date', function() {
        it('will return false if the preload date is in the future but is a demo, beta, or trial', function(done) {
            GameAutomaticViolatorTestsFactory
                .preloadOn('preloadonfuturedemobetatrial')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('preloadon after release date', function() {
        it('will return false if the preload date has past', function(done) {
            GameAutomaticViolatorTestsFactory
                .preloadOn('preloadonpast')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('preloadAvailable between download and release date', function() {
        it('will ask the client for preload state - it will synchronize with the preload now button', function(done) {
            GameAutomaticViolatorTestsFactory
                .preloadAvailable('preloadavailable')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('releasesOn when preload and release dates are the same and the release date is in the future', function() {
        it('will return true', function(done) {
            GameAutomaticViolatorTestsFactory
                .releasesOn('releasesonfuture')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('releasesOn when preload and release dates are the same and the release date is in the past', function() {
        it('will return false', function(done) {
            GameAutomaticViolatorTestsFactory
                .releasesOn('releasesonpast')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('releasesOn when the is no preload date and the releasedate is in the future', function() {
        it('will return true', function(done) {
            GameAutomaticViolatorTestsFactory
                .releasesOn('releasesonfuturenopreload')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('releasesOn when the data is incomplete to make a decision', function() {
        it('will return false', function(done) {
            GameAutomaticViolatorTestsFactory
                .releasesOn('noop')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('An update is available for a users game', function() {
        it('will ask the client for update status and return', function(done) {
            GameAutomaticViolatorTestsFactory
                .updateAvailable('updateavailable')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A non trial game entitlement is expiring in the future', function() {
        it('will check the entitlement termination date', function(done) {
            GameAutomaticViolatorTestsFactory
                .gameExpires('gameexpiresfuture')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A trial game entitlement that is expiring in the future', function() {
        it('will suppress this message', function(done) {
            GameAutomaticViolatorTestsFactory
                .gameExpires('gameexpiresfuturetrial')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A non trial game entitlement has expired', function() {
        it('will check the entitlement termination date', function(done) {
            GameAutomaticViolatorTestsFactory
                .gameExpires('gameexpirespast')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A trial game has never been played', function() {
        it('will return true if the game is a trial and there is no termination date set yet', function(done) {
            GameAutomaticViolatorTestsFactory
                .trialNotActivated('trialnotactivated')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A trial game is in progress and not yet expired', function() {
        it('will return true if the game is a trial and the termination date is in the future', function(done) {
            GameAutomaticViolatorTestsFactory
                .trialNotExpired('trialnotexpired')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A trial game has expired', function() {
        it('will return true if the game is a trial and the termination date is in the past', function(done) {
            GameAutomaticViolatorTestsFactory
                .trialExpired('trialexpired')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A new dlc is available', function() {
        it('returns true if: Game is a full game or full game plus expansion, Addon is unowned, Addon is purchasable, Addon is downloadable, Addon is released in the last 7 days', function(done) {
            GameAutomaticViolatorTestsFactory
                .newDlcAvailable('newdlcavailable')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A new dlc is available', function() {
        it('returns false after 7 days of showing', function(done) {
            GameAutomaticViolatorTestsFactory
                .newDlcAvailable('newdlcavailablepast7days')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A new game expansion is available', function() {
        it('returns true if: Game is a full game or full game plus expansion, Expansion is unowned, Expansion is purchasable, Expansion is downloadable, Expansion is released in the last 28 days', function(done) {
            GameAutomaticViolatorTestsFactory
                .newDlcExpansionAvailable('newdlcexpansionavailable')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A new game expansion is available', function() {
        it('returns false after 28 days of showing', function(done) {
            GameAutomaticViolatorTestsFactory
                .newDlcExpansionAvailable('newdlcexpansionavailablepast28days')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A new game addon or expansion is ready to install', function() {
        it('returns true if: Game is a full game or full game plus expansion, Expansion or addon is owned, Expansion or addon is downloadable, Expansion or addon is released in the last 7 days', function(done) {
            GameAutomaticViolatorTestsFactory
                .newDlcReadyForInstall('newdlcreadyforinstall')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('A new game addon or expansion is ready to install', function() {
        it('returns false after 7 days of showing', function(done) {
            GameAutomaticViolatorTestsFactory
                .newDlcReadyForInstall('newdlcreadyforinstall7days')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });

    describe('New DLC has been installed and has not been played', function() {
        it('should return true', function (done){
            GameAutomaticViolatorTestsFactory
                .newDlcInstalled('newdlcinstalled')
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });
    describe('New DLC has been installed but is more than seven days out', function() {
        it('should return false', function (done){
            GameAutomaticViolatorTestsFactory
                .newDlcInstalled('newdlcinstalled7days')
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(error){
                    fail(error);
                    done();
                });
        });
    });
});