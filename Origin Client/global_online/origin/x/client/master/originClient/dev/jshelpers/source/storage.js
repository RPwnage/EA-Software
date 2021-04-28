/*********************************
Copyright (c) 2012, Electronic Arts Inc. All rights reserved.
JavaScript functions to be utilized with new authentication flow.
*/

// To be called upon onSubmit when in EADP login window
function passCredentials(userId, password) {
    console.log('passCredentials: userId = ' + userId + ' password = ' + password);

    // pass credentials to OriginClient
    window.authenticationJsHelper.passCredentials(userId, password);
}

// Store data in web storage using a hash of userId+password for the key
function storeCredentialedData(data) {
    console.log('storeCredentialedData: ' + data);
    storeKeyedData(getCredentialsKey(), data);
}

// Store data in web storage using a specified key
function storeKeyedData(key, data) {
    console.log('storeKeyedData: key = ' + key + " data = " + data);
    if (key) {
        localStorage[key] = data;
    }
}

// Load data from web storage that was stored using a hash of userId+password for the key
function loadCredentialedData() {
    var credentials = loadKeyedData(getCredentialsKey());
    window.authenticationJsHelper.setCredentialedData(credentials);

    // TODO: remove test code
    document.getElementById("TestLoadResult").innerHTML = credentials;
}

// Load data from web storage that was stored using a specified key
function loadKeyedData(key) {
    var data = "";
    if (key) {
        data = localStorage[key];
    }

    return data;
}

// Remove data in web storage that was stored using a hash of userId+password for the key
function clearCredentialedData() {
    clearKeyedData(getCredentialsKey());
    window.authenticationJsHelper.resetCredentials();
}

// Remove data from web storage that was sored using a specified key
function clearKeyedData(key) {
    if (key) {
        localStorage[key] = "";
    }
}

// Create a key for web storage by hashing userId+password
function getCredentialsKey() {
    var key = "";
    var id = window.authenticationJsHelper.userId();
    var password = window.authenticationJsHelper.password();
    var environmentName = window.authenticationJsHelper.environmentName();

    console.log('getCredentialsKey: id = ' + id + ', password = ' + password + ', envName = ' + environmentName);

    if (id && password) {
	var shaObj = null;
	if (window.authenticationJsHelper.isProductionEnvironment()) {
            shaObj = new jsSHA(id + password, "ASCII");
	} else {
            console.log('getCredentialsKey: appending env name to key');
            shaObj = new jsSHA(id + environmentName + password, "ASCII");
        }
        key = shaObj.getHash("SHA-512", "HEX");
    }

    return key;
}