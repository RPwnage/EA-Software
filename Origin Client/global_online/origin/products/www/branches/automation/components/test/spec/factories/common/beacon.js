/**
 * Jasmine functional test
 */

'use strict';

describe('Beacon Factory', function() {
    var beaconFactory,
        stubResolveTrue = Promise.resolve(true),
        stubResolveFalse = Promise.resolve(false),
        stubResolveUndefined = Promise.resolve(undefined);

    beforeEach(function() {
        window.OriginComponents = {};
        angular.mock.module('origin-components');
        window.Origin = {
            beacon: {
                installed: function() {},
                installable: function() {},
                installableOnPlatform: function() {},
                running: function() {}
            },
            client: {
                isEmbeddedBrowser: function() {},
                info: {
                    version: function() {}
                }
            }
        };

        module(function($provide) {
            $provide.factory('ComponentsLogFactory', function(){
                return {
                    log: function(){}
                };
            });
        });
    });

    beforeEach(inject(function(_BeaconFactory_) {
        beaconFactory = _BeaconFactory_;
    }));

    describe('isInstalled', function() {
        it('Will be true if an instance of the Origin client is installed and the user is in an embedded browser', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(true);

            beaconFactory.isInstalled()
                .then(function(data) {
                    expect(window.Origin.client.isEmbeddedBrowser).toHaveBeenCalled();
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will be true if an instance of the Origin client is installed on the machine, but the user is in an external browser', function(done) {
            spyOn(window.Origin.beacon, 'installed').and.returnValue(Promise.resolve('10.99.0.39802'));

            beaconFactory.isInstalled()
                .then(function(data) {
                    expect(window.Origin.beacon.installed).toHaveBeenCalled();
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will be false if an instance of the Origin client is not installed on the machine', function(done) {
            spyOn(window.Origin.beacon, 'installed').and.returnValue(Promise.resolve(undefined));

            beaconFactory.isInstalled()
                .then(function(data) {
                    expect(window.Origin.beacon.installed).toHaveBeenCalled();
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('isRunning', function() {
        it('Will be true if an instance of the Origin client is actively running', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(true);

            beaconFactory.isRunning()
                .then(function(data) {
                    expect(window.Origin.client.isEmbeddedBrowser).toHaveBeenCalled();
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will be true if an instance of the Origin client is actively running, but a user is in an external browser', function(done) {
            spyOn(window.Origin.beacon, 'running').and.returnValue(Promise.resolve(true));

            beaconFactory.isRunning()
                .then(function(data) {
                    expect(window.Origin.beacon.running).toHaveBeenCalled();
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will be false if an instance of the Origin client is not installed on the machine', function(done) {
            spyOn(window.Origin.beacon, 'running').and.returnValue(Promise.resolve(false));

            beaconFactory.isRunning()
                .then(function(data) {
                    expect(window.Origin.beacon.running).toHaveBeenCalled();
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('isInstallable', function() {
        it('Will be true if the SPA is already running in a client context', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(true);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.isInstallable()
                .then(function(data) {
                    expect(window.Origin.client.isEmbeddedBrowser).toHaveBeenCalled();
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will check the User Agent for installability', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveTrue);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.isInstallable()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will check the running flag in case user agent is messed up or out of date', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveTrue);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.isInstallable()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will check the installed flag in case user agent is messed up or out of date', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(Promise.resolve('10.99.0.39802'));

            beaconFactory.isInstallable()
                .then(function(data) {
                    expect(data).toBe(true);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('In case the client is not installable on the current machine', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.isInstallable()
                .then(function(data) {
                    expect(data).toBe(false);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('isinstallableOnPlatform', function(done) {
        it('will pass through to the jssdk functionality', function(done) {
            function stubResolveTrue() {
                return Promise.resolve(true);
            };

            spyOn(window.Origin.beacon, 'installableOnPlatform').and.callFake(stubResolveTrue);

            beaconFactory.isInstallableOnPlatform('PCWIN')
                .then(function(data) {
                    expect(window.Origin.beacon.installableOnPlatform).toHaveBeenCalledWith('PCWIN');
                    done();
                });
        });
    });

   describe('getState', function() {
        it('Expect a running state if the SPA is already running in a client context', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(true);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.getState()
                .then(function(data) {
                    expect(window.Origin.client.isEmbeddedBrowser).toHaveBeenCalled();
                    expect(data).toBe(beaconFactory.states.RUNNING);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will check client is on a compatible platform with the minimum OS requirements to install', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveTrue);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.getState()
                .then(function(data) {
                    expect(data).toBe(beaconFactory.states.INSTALLABLE);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('In the case where the client is running, but the user is on an external browser', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveTrue);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.getState()
                .then(function(data) {
                    expect(data).toBe(beaconFactory.states.RUNNING);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('In the case the client is installed and advertising a version number', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(Promise.resolve('10.99.0.39802'));

            beaconFactory.getState()
                .then(function(data) {
                    expect(data).toBe(beaconFactory.states.INSTALLED);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('In case the client is not installable on the current machine', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(false);
            spyOn(window.Origin.beacon, 'installable').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'running').and.returnValue(stubResolveFalse);
            spyOn(window.Origin.beacon, 'installed').and.returnValue(stubResolveUndefined);

            beaconFactory.getState()
                .then(function(data) {
                    expect(data).toBe(beaconFactory.states.INCOMPATIBLE);
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });

    describe('getClientVersion', function(done) {
        it('Will be true if an instance of the Origin client is installed and the user is in an embedded browser', function(done) {
            spyOn(window.Origin.client, 'isEmbeddedBrowser').and.returnValue(true);
            spyOn(window.Origin.client.info, 'version').and.returnValue('10.99.0.39801');

            beaconFactory.getClientVersion()
                .then(function(data) {
                    expect(window.Origin.client.info.version).toHaveBeenCalled();
                    expect(data).toBe('10.99.0.39801');
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });

        it('Will be true if an instance of the Origin client is installed and the user is in an external browser', function(done) {
            spyOn(window.Origin.beacon, 'installed').and.returnValue(Promise.resolve('10.99.0.39802'));

            beaconFactory.getClientVersion()
                .then(function(data) {
                    expect(window.Origin.beacon.installed).toHaveBeenCalled();
                    expect(data).toBe('10.99.0.39802');
                    done();
                })
                .catch(function(err) {
                    fail(err);
                    done();
                });
        });
    });
});