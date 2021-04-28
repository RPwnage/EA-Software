describe('Origin Defines API', function() {

    beforeEach(function() {
        jasmine.addMatchers(matcher);
    });

    it('Origin.defines.http', function() {
        var httpResponseCodesObject = Origin.defines.http;
		
		expected_httpResponseCodesObject = Object({
			SUCCESS_200: 200,
			REDIRECT_302_FOUND: 302,
			ERROR_400_BAD_REQUEST: 400,
			ERROR_401_UNAUTHORIZED: 401,
			ERROR_403_FORBIDDEN: 403,
			ERROR_404_NOTFOUND: 404,
			ERROR_UNEXPECTED: -99
		});

        expect(httpResponseCodesObject).toEqual(expected_httpResponseCodesObject);
    });
	
	it('Origin.defines.login', function() {
        var loginTypesObject = Origin.defines.login;
		
		expected_loginTypesObject = Object({ 
			APP_INITIAL_LOGIN: 'login_app_initial', 
			APP_RETRY_LOGIN: 'login_app_retry', 
			AUTH_INVALID: 'login_auth_invalid', 
			POST_OFFLINE: 'login_post_offline' 
		});

        expect(loginTypesObject).toEqual(expected_loginTypesObject);
    });
});

