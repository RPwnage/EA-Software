'use strict';

var fs = require('fs');

var environment = 'prod';
var fileContentLines;
var overrideMappings = {};
var overrideJson = {
    env: 'prod',
    urls: {}
};
var output = [];


function setOverrideMappings() {
    overrideMappings = [{
        override: 'promourl',
        value: 'PromoV2URL'
    }, {
        override: 'overridecustomersupportpageurl',
        value: 'CustomerSupportURL'
    }];
}

function parseOutEnvironment() {
    var numlines = fileContentLines.length;
    var i;
    for (i = 0; i < numlines; i++) {
        var line = fileContentLines[i];
        var sectionndx = line.indexOf('[Connection]');
        if (sectionndx < 0) {
            continue;
        }

        //here if we're [Connection]
        var j;
        for (j = i + 1; j < numlines; j++) {
            line = fileContentLines[j];
            sectionndx = line.indexOf('[');
            if (sectionndx >= 0) //moved into another section
                return '';

            var ndx = line.indexOf('EnvironmentName=');
            if (ndx >= 0) {
                var commentndx = line.indexOf(';');
                if (commentndx !== -1 && commentndx < ndx) {
                    continue;
                }

                var tokenlist = line.split('=');
                return tokenlist[1];
            }
        }
    }
    return '';
}

function parseKeyValue(overrideLine) {
    var tokenlist = overrideLine.split('=');
    var concatTokenList = [];

    for (var i = 0; i < tokenlist.length; i++) {
        tokenlist[i] = tokenlist[i].trim();
    }
    if (tokenlist.length == 2) {
        concatTokenList = tokenlist;
    } else { 
        //we only want the first equal as the splitting point, concatenate all other = back in
        concatTokenList[0] = tokenlist [0];
        concatTokenList[1] = tokenlist [1];
        for (var j = 2; j < tokenlist.length; j++) {
            concatTokenList [1] += '=' + tokenlist[j];
        }
    }

    return concatTokenList;
}

function parseOverrideUrl() {
    var numlines = fileContentLines.length;
    var i;
    var firstOverride = true;
    var foundUrlSection = false;
    var jsonObject = {};
    for (i = 0; i < numlines; i++) {
        var line = fileContentLines[i];
        if (line.toLowerCase() === '[urls]') {
            foundUrlSection = true;
        }
        var equalndx = line.indexOf('=');

        if (equalndx < 0) { //not an assignment
            continue;
        }

        //check and see if it's a comment
        var commentndx = line.indexOf(';');
        if (commentndx !== -1 && commentndx < equalndx) {
            continue;
        }

        if (foundUrlSection) {
            var found = false,
                jsonKeyIndex = -1,
                k = 0,
                jsonString = '',
                keyValueList = parseKeyValue(line);

            //see if this has been tagged as a JSON object 
            jsonKeyIndex = keyValueList[0].indexOf(':JSON');
            if (jsonKeyIndex > -1) {

                //create json objects from EACore.ini entries
                /*
                
                    --sample EACore.ini entry--
                    [Urls]
                    routeUrls.mygames.default:JSON=mygames/views/mygames2.html
                    routeUrls.mygames.anon:JSON=mygames/views/mygamesanon2.html
                    routeUrls.mygames.offline:JSON=mygames/views/mygamesoffline.html
                 */
                keyValueList[0] = keyValueList[0].slice(0, jsonKeyIndex);

                var list = keyValueList[0].split('.');
                var currObj = jsonObject;
                for (var z = 0; z < list.length; z++) {
                    if (!currObj[list[z]]) {
                        currObj[list[z]] = {};
                    }
                    if (z === (list.length - 1)) {
                        currObj[list[z]] = keyValueList[1];
                    } else
                        currObj = currObj[list[z]];
                }
                output.push('<li>', keyValueList[0], ': ', keyValueList[1], '</li>');
            } else {
                for (var j = 0; j < overrideMappings.length; j++) {
                    if (keyValueList[0].toLowerCase() === overrideMappings[j].override.toLowerCase()) {
                        overrideJson.urls[overrideMappings[j].value] = keyValueList[1];
                        if (firstOverride === true) {
                            output.push('<p><strong>', 'Override(s) found:', '</strong></p>');
                            firstOverride = false;
                        }
                        output.push('<li>', overrideMappings[j].value, ': ', keyValueList[1], '</li>');
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    overrideJson.urls[keyValueList[0]] = keyValueList[1];
                    output.push('<li>', keyValueList[0], ': ', keyValueList[1], '</li>');
                }
            }
        }

    }

    //assign it to the override url object  
    for (var p in jsonObject) {
        if (jsonObject.hasOwnProperty(p)) {
            overrideJson.urls[p] = jsonObject[p];
        }
    }
}

function parseINI() {
    var envSet = parseOutEnvironment();
    if (envSet.length > 0) {
        environment = envSet;
        overrideJson.env = envSet;
    }

    output.push('<p><strong>', 'Environment: ', environment, '</strong></p>');

    parseOverrideUrl();
    document.getElementById('list').innerHTML = '<ul>' + output.join('') + '</ul>';
}



function handleFileSelect(evt) {

    evt.stopPropagation();
    evt.preventDefault();

    var files = evt.dataTransfer.files; // FileList object.

    var f = files[0];

    processINI(f.path);
}

function processINI(filename) {
    output = []; //clear any previous messages
    output.push('<p><strong>', 'loading overrides from: ', filename, '</strong></p>');
    document.getElementById('list').innerHTML = '<ul>' + output.join('') + '</ul>';

    loadINI(filename);
    if (fileContentLines.length > 0) {
        //save out the filename
        var fd = fs.writeFileSync('inifile.txt', filename);
        parseINI();
        saveOverrideJson();
    }
}

function loadINI(filename) {
    console.log('ParseINI:');

    //!!!!!! NEED TO ADD ERROR HANDLING
    var data = fs.readFileSync(filename, 'utf-8');

    //split it out into lines
    fileContentLines = data.split(/\r\n|\r|\n/g);
}

function saveOverrideJson() {
    //there is something to write out
    if (Object.keys(overrideJson).length > 0) {
        var fd = fs.writeFileSync('overrides.json', JSON.stringify(overrideJson));
    }
}


function handleDragOver(evt) {
    evt.stopPropagation();
    evt.preventDefault();
    evt.dataTransfer.dropEffect = 'copy'; // Explicitly show this is a copy.
}

function showExistingOverride() {
    console.log('showExistingOverride');
    var existFound = false;
    try {
        var fname = fs.readFileSync('inifile.txt', 'utf-8');
        processINI(fname);
    } catch (e) {
        //couldn't read the eacore.ini file path, so remove overrides.json if it exists
        var exists = fs.existsSync('overrides.json');
        if (exists) {
            fs.unlinkSync('overrides.json');
        }
        output.push('<p><strong>', 'No override file specified: ', '</strong></p>');
    }
    document.getElementById('list').innerHTML = '<ul>' + output.join('') + '</ul>';
}

function launch() {
    var open = require("open");
    var url;
    var exists = false;
    //check and see if override exists, if so, append parameter to url
    try {
        var data = fs.openSync('overrides.json', 'r');
        if (data) {
            exists = true;
        }
    } catch (e) {
        exists = false;
    }

    var inputUrl = document.getElementById('urlBox').value;

    if (inputUrl.length) {
        url = inputUrl;
        if (exists) {
            url += '?override=true';
        }

        open(url);
    } else {
        alert('please enter a url');
    }

}

setOverrideMappings();

//check and see if override.json already exists; if so, display that
showExistingOverride();


// Setup the dnd listeners.
var dropZone = document.getElementById('drop_zone');
dropZone.addEventListener('dragover', handleDragOver, false);
dropZone.addEventListener('drop', handleFileSelect, false);

var http = require('http'),
    urlmodule = require('url'),
    path = require('path'),
    port = process.argv[2] || 1234;

http.createServer(function(request, response) {

    // Set CORS headers
    response.setHeader('Access-Control-Allow-Origin', '*');
    response.setHeader('Access-Control-Request-Method', '*');
    response.setHeader('Access-Control-Allow-Methods', 'OPTIONS, GET');
    response.setHeader('Access-Control-Allow-Headers', '*');
    if (request.method === 'OPTIONS') {
        response.writeHead(200);
        response.end();
        return;
    }

    var uri = urlmodule.parse(request.url).pathname,
        filename = path.join(process.cwd(), uri);

    path.exists(filename, function(exists) {
        if (!exists) {
            response.writeHead(404, {
                'Content-Type': 'text/plain'
            });
            response.write('404 Not Found\n');
            response.end();
            return;
        }

        if (fs.statSync(filename).isDirectory()) filename += '/index.html';

        fs.readFile(filename, 'binary', function(err, file) {
            if (err) {
                response.writeHead(500, {
                    'Content-Type': 'text/plain'
                });
                response.write(err + '\n');
                response.end();
                return;
            }

            response.writeHead(200);
            response.write(file, 'binary');
            response.end();
        });
    });
}).listen(parseInt(port, 10));