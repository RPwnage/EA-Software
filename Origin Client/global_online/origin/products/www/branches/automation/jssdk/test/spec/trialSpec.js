describe('Origin Trial API', function() {
	
	beforeEach(function() {
		jasmine.addMatchers(matcher);
	});
	
	it('Origin.trial.getTime()', function(done) {
		
		var asyncTicker = new AsyncTicker();
		asyncTicker.tick(1000, done, 'Expected getTime() to be resolved');
		
		Origin.trial.getTime('1031469_oa').then(function(result) {
			expect(result.hasTimeLeft).toBeTruthy('Expected the hasTimeLeft property of the getTimeResponseObject to return true');
			expect(result.leftTrialSec).toEqual(30600, 'Expected the leftTrialSec property of the getTimeResponseObject to return the correct number of seconds');
			expect(result.totalTrialSec).toEqual(37200, 'Expected the totalTrialSec property of the getTimeResponseObject to return the correct number of seconds');
			expect(result.totalGrantedSec).toEqual(1200, 'Expected the totalGrantedSec property of the getTimeResponseObject to return the correct number of seconds');
			asyncTicker.clear(done);
		}).catch(function(error) {
			expect().toFail(error.message);
			asyncTicker.clear(done);
		});
	});
});