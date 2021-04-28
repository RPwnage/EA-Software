describe('Origin VoiceBridge API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.voice.answerCall()', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICECONNECTED should be triggered');

        expect(Origin.voice.answerCall()).toEqual(undefined);

    });

    it('Origin.voice.audioInputDevices()', function(done) {

        var expected = {
            'devices': 0
        };

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.voice.audioInputDevices().then(function(result) {
            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });


    });



    it('Origin.voice.ignoreCall()', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICEDISCONNECTED should be triggered');

        expect(Origin.voice.ignoreCall(0)).toEqual(undefined);

    });

    it('Origin.voice.joinVoice()', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICECONNECTED should be triggered');

        expect(Origin.voice.joinVoice(0, [])).toEqual(undefined);

    });

    it('Origin.voice.leaveVoice()', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICEDISCONNECTED should be triggered');

        expect(Origin.voice.leaveVoice(0)).toEqual(undefined);

    });

    it('Origin.voice.muteSelf()', function() {

        expect(Origin.voice.muteSelf()).toEqual(undefined);

    });

    it('Origin.voice.networkQuality()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.voice.networkQuality().then(function(result) {

            expect(result).toEqual(0);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });


    });

    it('Origin.voice.playIncomingRing()', function() {

        expect(Origin.voice.playIncomingRing()).toEqual(undefined);

    });

    it('Origin.voice.playOutgoingRing()', function() {

        expect(Origin.voice.playOutgoingRing()).toEqual(undefined);

    });

    it('Origin.voice.selectedAudioInputDevice()', function(done) {

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.voice.selectedAudioInputDevice().then(function(result) {

            expect(result).toEqual('device');
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });

    });

    it('Origin.voice.showSurvey()', function(done) {

        expected = {
            'channelId': 0
        };

        var asyncTicker = new AsyncTicker();

        asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');

        Origin.voice.showSurvey(0).then(function(result) {

            expect(result).toEqual(expected);
            asyncTicker.clear(done);

        }).catch(function(error) {
            expect().toFail(error.message);
            asyncTicker.clear(done);
        });
    });

    it('Origin.voice.showToast()', function() {

        expect(Origin.voice.showToast(0, 1, 2)).toEqual(undefined);

    });

    it('Origin.voice.stopIncomingRing()', function() {

        expect(Origin.voice.stopIncomingRing()).toEqual(undefined);

    });

    it('Origin.voice.stopOutgoingRing()', function() {

        expect(Origin.voice.stopOutgoingRing()).toEqual(undefined);

    });

    it('Origin.voice.supported()', function() {

        expect(Origin.voice.supported()).toEqual(true);

    });

    it('Origin.voice.unmuteSelf()', function() {

        expect(Origin.voice.unmuteSelf()).toEqual(undefined);

    });

    it('Origin.voice.isSupportedBy()', function() {

        expect(Origin.voice.isSupportedBy(0)).toBeTruthy();

    });

});
