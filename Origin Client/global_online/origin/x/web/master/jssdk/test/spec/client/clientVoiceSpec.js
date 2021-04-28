describe('Origin Client Voice API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.events.CLIENT_VOICE_DEVICEADDED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DEVICEADDED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DEVICEADDED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DEVICEADDED should be triggered');

        window.OriginVoice.deviceAdded([]);

    });
	
	it('Origin.events.CLIENT_VOICE_DEVICEREMOVED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DEVICEREMOVED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DEVICEREMOVED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DEVICEREMOVED should be triggered');

        window.OriginVoice.deviceRemoved([]);

    });
	
	it('Origin.events.CLIENT_VOICE_DEFAULTDEVICECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DEFAULTDEVICECHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DEFAULTDEVICECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DEFAULTDEVICECHANGED should be triggered');

        window.OriginVoice.defaultDeviceChanged([]);

    });
	
	it('Origin.events.CLIENT_VOICE_DEVICECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DEVICECHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DEVICECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DEVICECHANGED should be triggered');

        window.OriginVoice.deviceChanged([]);

    });
	
	it('Origin.events.CLIENT_VOICE_VOICELEVEL', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICELEVEL, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICELEVEL, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICELEVEL should be triggered');

        window.OriginVoice.voiceLevel([]);

    });

	it('Origin.events.CLIENT_VOICE_UNDERTHRESHOLD', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_UNDERTHRESHOLD, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_UNDERTHRESHOLD, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_UNDERTHRESHOLD should be triggered');

        window.OriginVoice.underThreshold([]);

    });
	
	it('Origin.events.CLIENT_VOICE_OVERTHRESHOLD', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_OVERTHRESHOLD, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_OVERTHRESHOLD, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_OVERTHRESHOLD should be triggered');

        window.OriginVoice.overThreshold([]);

    });
	
	it('Origin.events.CLIENT_VOICE_VOICECONNECTED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICECONNECTED should be triggered');

        window.OriginVoice.voiceConnected([]);

    });
	
	it('Origin.events.CLIENT_VOICE_VOICEDISCONNECTED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICEDISCONNECTED should be triggered');

        window.OriginVoice.voiceDisconnected([]);

    });
	
	it('Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE should be triggered');

        window.OriginVoice.enableTestMicrophone([]);

    });
	
	it('Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE should be triggered');

        window.OriginVoice.disableTestMicrophone([]);

    });
	
	it('Origin.events.CLIENT_VOICE_CLEARLEVELINDICATOR', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_CLEARLEVELINDICATOR, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_CLEARLEVELINDICATOR, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_CLEARLEVELINDICATOR should be triggered');

        window.OriginVoice.clearLevelIndicator([]);

    });
	
	it('Origin.events.CLIENT_VOICE_VOICECALLEVENT', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICECALLEVENT, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICECALLEVENT, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_VOICECALLEVENT should be triggered');

        window.OriginVoice.voiceCallEvent([]);

    });
	
	it('Origin.client.voice.audioInputDevices()', function() {
			
		expected = {
			'devices': 0
		};
		
		expect(Origin.client.voice.audioInputDevices()).toEqual(expected);

    });
	
	it('Origin.client.voice.audioOutputDevices()', function() {
			
		expected = {
			'devices': 1
		};
		
		expect(Origin.client.voice.audioOutputDevices()).toEqual(expected);

    });
	
	it('Origin.client.voice.changeInputDevice(), Origin.events.CLIENT_VOICE_DEVICECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DEVICECHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DEVICECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DEVICECHANGED should be triggered');
		
        Origin.client.voice.changeInputDevice(0).then(function(result) {
			
			expected = { 
				'device': 0 
			};
		
			expect(result).toEqual(expected);
		
		}).catch(function(error) {
				
			expect().toFail('Expect reject() to be called, ' + error.message);
			asyncTicker.clear(done);
				
		});
    });
	
	it('Origin.client.voice.changeOutputDevice(), Origin.events.CLIENT_VOICE_DEVICECHANGED', function(done) {

        var asyncTicker = new AsyncTicker();

        var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DEVICECHANGED, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_DEVICECHANGED, cb);

        asyncTicker.tick(1000, done, 'event Origin.events.CLIENT_VOICE_DEVICECHANGED should be triggered');
		
        Origin.client.voice.changeOutputDevice(0).then(function(result) {
			
			expected = { 
				'device': 0 
			};
		
			expect(result).toEqual(expected);
		
		}).catch(function(error) {
				
			expect().toFail('Expect reject() to be called, ' + error.message);
			asyncTicker.clear(done);
				
		});
    });
	
	it('Origin.client.voice.isInVoice()', function() {
		
        expect(Origin.client.voice.isInVoice()).toBeTruthy();
		
    });
	
	it('Origin.client.voice.isSupportedBy()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.isSupportedBy(0).then(function(result){
			
			expect(result).toBeTruthy();
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.joinVoice(), Origin.events.CLIENT_VOICE_VOICECONNECTED', function(done) {
		
		var asyncTicker = new AsyncTicker();

		var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICECONNECTED, cb);
	
		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.joinVoice(0, []).then(function(result){
			
			expected = {
				'id': 0,
				'participants': []
			};
			
			expect(result).toEqual(expected);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.leaveVoice(), Origin.events.CLIENT_VOICE_VOICEDISCONNECTED', function(done) {
		
		var asyncTicker = new AsyncTicker();
		
		var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

            expect(result).toEqual({});
            asyncTicker.clear(done);

        };

        Origin.events.on(Origin.events.CLIENT_VOICE_VOICEDISCONNECTED, cb);

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.leaveVoice(0).then(function(result){
			
			expected = {
				'id': 0,
			};
			
			expect(result).toEqual(expected);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.muteSelf()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.muteSelf().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.unmuteSelf()', function(done) {
				
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.unmuteSelf().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.networkQuality()', function() {
	
        expect(Origin.client.voice.networkQuality()).toEqual(0);
		
    });
	
	it('Origin.client.voice.playOutgoingRing()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.playOutgoingRing().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.playIncomingRing()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.playIncomingRing().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.selectedAudioInputDevice()', function() {
	
        expect(Origin.client.voice.selectedAudioInputDevice()).toEqual('device');

    });
	
	it('Origin.client.voice.selectedAudioOutputDevice()', function() {
		
		expect(Origin.client.voice.selectedAudioOutputDevice()).toEqual('device');
		
    });
	
	it('Origin.client.voice.setInVoiceSettings()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.setInVoiceSettings(true).then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.showToast()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.showToast(0, 1, 2).then(function(result){
			
			expected = {
				'event': 0,
				'originId': 1,
				'conversationId': 2
			};
			
			expect(result).toEqual(expected);
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.startVoiceChannel()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.startVoiceChannel().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.stopIncomingRing()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.stopIncomingRing().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.stopOutgoingRing()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.stopOutgoingRing().then(function(result){
			
			expect(result).toEqual({});
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.stopVoiceChannel()', function(done) {
		
		var asyncTicker = new AsyncTicker();

		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.client.voice.stopVoiceChannel().then(function(result){
			
			expect(result).toEqual(true);
			asyncTicker.clear(done);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.supported()', function() {

		expect(Origin.client.voice.supported()).toEqual(true);

    });
	
	it('Origin.client.voice.testMicrophoneStart(), Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE', function(done) {
		
		var asyncTicker = new AsyncTicker();
		
		var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };
		
		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.events.on(Origin.events.CLIENT_VOICE_ENABLETESTMICROPHONE, cb);
		
		Origin.client.voice.testMicrophoneStart().then(function(result){
			
			expect(result).toEqual(true);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
	
	it('Origin.client.voice.testMicrophoneStop(), Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE', function(done) {
		
		var asyncTicker = new AsyncTicker();

		var cb = function(result) {
            Origin.events.off(Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE, cb);

            expect(result).toEqual([]);
            asyncTicker.clear(done);

        };
		
		asyncTicker.tick(1000, done, 'Expect onResolve() to be executed');
	
        Origin.events.on(Origin.events.CLIENT_VOICE_DISABLETESTMICROPHONE, cb);
	
        Origin.client.voice.testMicrophoneStop().then(function(result){
			
			expect(result).toEqual(true);			
			
		}).catch(function(error){
			
			expect().toFail('Expect resolve() to be called, ' + error.message);
			asyncTicker.clear(done);
			
		});
    });
});

