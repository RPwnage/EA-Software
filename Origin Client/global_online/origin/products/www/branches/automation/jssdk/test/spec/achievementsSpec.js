describe('Origin Achievements API', function() {

	beforeEach(function() {
		jasmine.addMatchers(matcher);
	});

    it('Origin.achievments.userAchievements()', function(done) {
        var personaId = "myId";
        var achievementSet = "mySet";
        var locale = "enUS";
        var timeout = new AsyncTicker();

        timeout.tick(1000, done, 'userAchievements did not complete in 1000ms');
        Origin.achievements.userAchievements(personaId, achievementSet, locale).then(function(result) {
            var expected_result = ['achievement1', 'achievement2'];
            expect(result.achievements).toEqual(expected_result, 'Expected the result to be ' + expected_result);
            timeout.clear(done);
        }).catch(function(reason) {
            timeout.clear(done);
            expect().toFail(reason.message);
        });
    });

    it('Origin.achievments.userAchievementSets()', function(done) {
        var personaId = "myId";
        var locale = "enUS";
        var timeout = new AsyncTicker();

        timeout.tick(1000, done, 'userAchievementSets did not complete in 1000ms');
        Origin.achievements.userAchievementSets(personaId, locale).then(function(result) {
            expect(result.achievements).toEqual([], 'Expected achievements to be []');
            expect(result.expansions).toEqual([], 'Expected expansions to be []');
            expect(result.name).toEqual('my achievement set', 'Expected name to be my achievement set');
            expect(result.platform).toEqual('windows', 'Expected name to be windows');
            timeout.clear(done);
        }).catch(function(reason) {
            timeout.clear(done);
            expect().toFail(reason.message);
        });
    });

    it('Origin.achievments.userAchievementPoints()', function(done) {
        var personaId = "myId";
        var timeout = new AsyncTicker();

        timeout.tick(1000, done, 'userAchievementPoints did not complete in 1000ms');
        Origin.achievements.userAchievementPoints(personaId).then(function(result) {
            expect(result.points).toEqual(1337, 'Expected points to be 1337');
            timeout.clear(done);
        }).catch(function(reason) {
            timeout.clear(done);
            expect().toFail(reason.message);
        });
    });

    it('Origin.achievments.achievementSetReleaseInfo()', function(done) {
        var locale = "en-US";
        var timeout = new AsyncTicker();

        timeout.tick(1000, done, 'achievementSetReleaseInfo did not complete in 1000ms');
        Origin.achievements.achievementSetReleaseInfo(locale).then(function(result) {
            firstSet = result.achievementSets[0];
            sndSet = result.achievementSets[1];
            expect(firstSet.achievements).toEqual([], 'Expected achievements to be []');
            expect(firstSet.expansions).toEqual([], 'Expected expansions to be []');
            expect(firstSet.name).toEqual('my achievement set1', 'Expected achievements to be my achievement set1');
            expect(firstSet.platform).toEqual('AIX', 'Expected platform to be AIX');
            expect(sndSet.achievements).toEqual([], 'Expected achievements to be []');
            expect(sndSet.expansions).toEqual([], 'Expected expansions to be []');
            expect(sndSet.name).toEqual('my achievement set2', 'Expected achievements to be my achievement set1');
            expect(sndSet.platform).toEqual('FreeBSD', 'Expected platform to be FreeBSD');
            timeout.clear(done);
        }).catch(function(reason) {
            timeout.clear(done);
            expect().toFail(reason.message);
        });
    });
});
