/**
 * Jasmine functional test
 */

'use strict';

describe('Experimentation Factory', function() {
    var experimentFactory,
        stubResolveTrue = Promise.resolve(true),
        stubReject = Promise.reject('error');


    beforeEach(function() {
        window.OriginComponents = {};

        function Communicator() {}
        Communicator.prototype.fire = function() {};
        window.Experiment = {
            init: function () {},
            setUser: function () {},
            clearUser: function () {},
            setUserCustomDimension: function () {},
            inExperiment: function() {},
            clearCustomDimension: function() {},
            getUserCustomDimension: function() {}
        };

        angular.mock.module('eax-experiments');


        module(function($provide) {
            $provide.factory('ExperimentCommFactory', function(){
                return {
                    Communicator: Communicator
                };
            });
        });
    });

    beforeEach(inject(function(_ExperimentFactory_) {
        experimentFactory = _ExperimentFactory_;
    }));


    describe('init', function() {
        it('tests for correct calling of Experiment.init', function(done) {
            spyOn(window.Experiment, 'init').and.returnValue(stubResolveTrue);

            experimentFactory.init('dev', 'preview', 'usa', 'en-us')
                .then(function(result) {
                    expect(window.Experiment.init).toHaveBeenCalled();
                    expect(window.Experiment.init).toHaveBeenCalledWith(
                        'dev',
                        'preview',
                        'usa',
                        'en-us');
                    done();
                });
        });

        it ('tests for handling Experiment.init failing', function(done) {
            spyOn(window.Experiment, 'init').and.returnValue(stubReject);

            experimentFactory.init('dev', 'preview', 'usa', 'en-us')
                .then(function(result) {
                    expect(window.Experiment.init).toHaveBeenCalled();
                    expect(window.Experiment.init).toHaveBeenCalledWith(
                        'dev',
                        'preview',
                        'usa',
                        'en-us');
                    done();
                }).catch(function(error){
                    expect(window.Experiment.init).toHaveBeenCalled();
                    expect(window.Experiment.init).toHaveBeenCalledWith(
                        'dev',
                        'preview',
                        'usa',
                        'en-us');
                    done();
                })
        });

        it('test for inExperiment to return variant', function(done) {
            var customDimension = 'experiment1|variantA,my-home-video-button|showButton',
                inExpObj = {
                    result: true,
                    addedCustomDimension: true
                };

            spyOn(window.Experiment, 'inExperiment').and.returnValue(Promise.resolve(inExpObj));
            spyOn(window.Experiment, 'getUserCustomDimension').and.returnValue(customDimension);
            spyOn(experimentFactory.events, 'fire').and.callThrough();


            experimentFactory.inExperiment('expname', 'expvariant')
                .then(function(result) {
                    expect(result).toBe(true);
                    expect(experimentFactory.events.fire).toHaveBeenCalledWith('experiment:updateCustomDimension', customDimension);
                    done();
                }).catch(function(error){
                    fail(error);
                    done();
                });
        });

        it('test for inExperiment to fail', function(done) {
            var customDimension = 'experiment1|variantA,my-home-video-button|showButton',
                inExpObj = {
                    result: true,
                    addedCustomDimension: true
                };

            spyOn(window.Experiment, 'inExperiment').and.returnValue(stubReject);
            spyOn(window.Experiment, 'getUserCustomDimension').and.returnValue(customDimension);
            spyOn(experimentFactory.events, 'fire').and.callThrough();


            experimentFactory.inExperiment('expname', 'expvariant')
                .then(function(result) {
                    expect(result).toBe(false);
                    expect(experimentFactory.events.fire).not.toHaveBeenCalledWith('experiment:updateCustomDimension', customDimension);
                    done();
                }).catch(function(error){
                    fail(error);
                    done();
                });
        });

    });

});