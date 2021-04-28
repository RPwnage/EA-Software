$SchemaFilename = $Generator.NamespaceToFilename($Schema.Namespace).Replace('/','.')
$SchemaFilename + ".js"
"if (typeof(Origin) === 'undefined') {
    Origin = {};
}
Origin.SDK = (function () {
    ""use strict"";
    var connection,
        serviceMap = {},
        responseMap = {},
        eventMap = {},
        eventFallback,
        profile,

        ERROR_NOT_LOADED = ""ORIGIN_ERROR_CORE_NOTLOADED : The Origin desktop application is not connected."",
        ERROR_NO_RESPONSE = ""ORIGIN_ERROR_LSX_NO_RESPONSE : Origin didn\'t respond within the set timeout."",
        ERROR_INIT_SUCCESSFUL = ""ORIGIN_ERROR_SUCCESS : The connection with Origin is successfully established."",

"
    foreach($i in $Schema.ComplexTypes)
    {
        if($i.isEvent -eq $true)
        {
"        EVENT_" + $i.Name + " = """ + $i.SourceName + """,";
        } 
    }
""
  
    foreach($i in $Schema.SimpleTypes)
    {
	    if($i.Enumerations.Count -ne 0)
	    {
            if($i.Name -eq "Facility" -or $i.Name -eq "IGOWindow")
            {
		        for($e = 0; $e -lt $i.Enumerations.Count; $e++)
		        {
"        " + $i.Name.ToUpper() + "_" + $i.Enumerations[$e] + " = """ + $i.Enumerations[$e] + """," 
		    }
            ""
        }
	}
}
      
foreach($i in $Schema.ComplexTypes)
{
    if($i.facility -ne $null)
    {
"        COMMAND_" + $i.Name.ToUpper() + " = """ + $i.SourceName + ""","        
    }
}

"        COMMAND_SHOWIGOWINDOW = ""ShowIGOWindow"",

        ID = 0;

    function getId() {
        ID += 1;
        return ID;
    }


    function getService(facility) {
        return serviceMap[facility] ? serviceMap[facility] : facility;
    }

    function processConfig(config) {
        var fac, facility;
        for (fac in config.Services) {
            facility = config.Services[fac];
            serviceMap[facility.Facility] = facility.Name;
        }
    }

    function sendRequest(facility, func, obj, successFunc, errFunc, timeout) {
        var requestId = getId(),
            packet = { LSX: { Request: { recipient: getService(facility), id: requestId } } };
        packet.LSX.Request[func] = obj;

        if (connection !== undefined) {

            responseMap[requestId] = { sFunc: successFunc, eFunc: errFunc };

            setTimeout(function () {
                if (responseMap[requestId] !== undefined) {
                    errFunc({ Code: -1593311231, Description: ERROR_NO_RESPONSE });
                    delete responseMap[requestId];
                }
            }, timeout !== undefined ? timeout : 15000);

            if (connection.readyState === WebSocket.OPEN) {
                connection.send(JSON.stringify(packet));
            } else {
                errFunc({ Code: -1610481664, Description: ERROR_NOT_LOADED });
            }
        } else {
            errFunc({ Code: -1610481664, Description: ERROR_NOT_LOADED });
        }
    }

    function challengeResponse(contentId, title, multiplayerId, language, successFunc, errFunc, timeout) {
        sendRequest('EALS', 'ChallengeResponse', { 'version' : 1, 'ContentId' : contentId, 'Title' : title, 'MultiplayerId' : multiplayerId, 'Language' : language, 'Version' : '" + $Schema.Version + "' }, successFunc, errFunc, timeout);
    }

    function getConfig(successFunc, errFunc, timeout) {
        sendRequest('EbisuSDK', 'GetConfig', {}, successFunc, errFunc, timeout);
    }

    function getProfile(successFunc, errFunc, timeout) {
        sendRequest(FACILITY_PROFILE, COMMAND_GETPROFILE, { 'index': 0 }, successFunc, errFunc, timeout);
    }

    function registerEvent(event, func) {
        if (eventMap[event]) {
            eventMap[event].push(func);
            return true;
        }
        return false;
    }

    function initEvents() {
        eventFallback = undefined;

        eventMap = {"

        $list = @();
    foreach($i in $Schema.ComplexTypes)
    {
        if($i.isEvent -eq $true)
        {
        $list += "            " + $i.SourceName + ": []";
        }
    }

        $list -join ",`r`n";

    "        };
    }

    function handleMessage(e) {
        var packet = JSON.parse(e.data), resp, fs, evnt, prop, i, handlers;
        if (packet.LSX) {
            resp = packet.LSX.Response;
            if (resp) {
                fs = responseMap[resp.id];

                if (fs) {
                    if (resp.ErrorSuccess) {
                        if (resp.ErrorSuccess.Code >= 0) {
                            fs.sFunc(resp.ErrorSuccess);
                        } else {
                            fs.eFunc(resp.ErrorSuccess);
                        }
                    } else {
                        for (prop in resp) {
                            if (resp.hasOwnProperty(prop)) {
                                if (prop !== 'id' && prop !== 'sender') {
                                    fs.sFunc(resp[prop]);
                                    break;
                                }
                            }
                        }
                    }
                    delete responseMap[resp.id];
                }
                return;
            }
            evnt = packet.LSX.Event;
            if (evnt) {
                for (prop in evnt) {
                    if (evnt.hasOwnProperty(prop)) {
                        if (prop !== 'sender') {
                            handlers = eventMap[prop];
                            if (handlers) {
                                if (handlers.length > 0) {
                                    for (i in handlers) {
                                        if (handlers.hasOwnProperty(i)) {
                                            handlers[i](evnt[prop]);
                                        }
                                    }
                                    return;
                                }
                            }
                            if (eventFallback) {
                                eventFallback(prop, evnt[prop]);
                            }
                        }
                    }
                }
                return;
            }
        }
    }

    function convertToArray(data) {
		var res;
		if (Array.isArray(data)) {
			return data;
		} else {
			if( typeof(data) === 'string' ) {
                data = data.replace(/[\s,]+/gm, ' ');
                if (data.length > 0) {
				    res = data.split(' ');
				    if (res.length > 0 && res[0] !== '') {
					    return res;
				    }
                }
				return undefined;
			} else {
				return [ data ];
			}
		}
    }

    return {
       // Constants
"
foreach($i in $Schema.SimpleTypes)
{
	if($i.Enumerations.Count -ne 0)
	{
        if($i.Name -ne "Facility" -and $i.Name -ne "IGOWindow")
        {
		    for($e = 0; $e -lt $i.Enumerations.Count; $e++)
		    {
                if($i.EnumerationDocumentation[$e].Length -gt 0)
                {
"        // " + $i.EnumerationDocumentation[$e];  
                }
"        " + $i.Name.ToUpper() + "_" + $i.Enumerations[$e] + ": """ + $i.Enumerations[$e] + """," 
		    }
            ""
        }
	}
}

"
        // Functions
        startup: function (contentId, title, multiplayerId, language, connectionId) {
            return new Promise(function (fulfill, reject) {
                initEvents();
                connection = new WebSocket('ws://localhost:3217/origin/' + connectionId);
                connection.onopen = function () {
                    challengeResponse(contentId, title, multiplayerId, language, function () {
                        getConfig(function (config) {
                            processConfig(config);
                            getProfile(function (data){ 
                                profile = data; 
                                fulfill({ 'Code': 0, 'Description': ERROR_INIT_SUCCESSFUL });
                            }, reject);
                        }, reject);
                    }, reject);
                };
                connection.onerror = function () {
                    reject({ Code: -1610481664, Description: ERROR_NOT_LOADED });
                };
                connection.onmessage = handleMessage;
                connection.onclose = function () {
                    reject({ Code: -1610481664, Description: ERROR_NOT_LOADED });
                };
            });
        },
        shutdown: function () {
            connection.close();
            ID = 0;
            responseMap = {};
            serviceMap = {};
        },
        getDefaultUser: function () {
            return profile ? profile.UserId : 0;
        },
        getDefaultPersona: function () {
            return profile ? profile.PersonaId : 0
        },
        // IGO Functions go here.
        //sendRequest(facility, COMMAND_SHOWIGOWINDOW, { 'UserId': UserId, 'WindowId': WindowId, 'Show': Show, 'Flags': Flags, 'ContentId': ContentId, 'TargetId': TargetId, 'String': String, 'Categories': Categories, 'MasterTitleIds': MasterTitleIds, 'Offers': Offers }, successFunc, errFunc, timeout);
        showProfileUI: function (UserId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': UserId, 'WindowId': IGOWINDOW_PROFILE, 'Show': true}, fulfill, reject, timeout);
            });
        },
        showFriendsProfileUI: function (userId, friendId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_PROFILE, 'Show': true, 'TargetId': convertToArray(friendId) }, fulfill, reject, timeout);
            });
        },
        showBrowserUI: function (flags, url, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'WindowId': IGOWINDOW_BROWSER, 'Show': true, 'Flags': flags, 'String': url}, fulfill, reject, timeout);
            });
        },
        showFriendsUI: function (userId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_FRIENDS, 'Show': true}, fulfill, reject, timeout);
            });
        },
        showFindFriendsUI: function (userId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_FIND_FRIENDS, 'Show': true}, fulfill, reject, timeout);
            });
        },
        showChangeAvatarUI: function (userId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_CHANGE_AVATAR, 'Show': true}, fulfill, reject, timeout);
            });
        },
        showRequestFriendUI: function (userId, friendId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_FRIEND_REQUEST, 'Show': true, 'TargetId': convertToArray(friendId) }, fulfill, reject, timeout);
            });
        },
        showcomposeChatUI: function (userId, friendId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_COMPOSE_CHAT, 'Show': true, 'TargetId': convertToArray(friendId) }, fulfill, reject, timeout);
            });
        },
        showInviteUI: function (userId, friendIds, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_INVITE, 'Show': true, 'TargetId': convertToArray(friendIds) }, fulfill, reject, timeout);
            });
        },
        showAchievementUI: function (userId, personaId, gameId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_ACHIEVEMENTS, 'Show': true, 'TargetId': ConverToArray(personaId), 'String': gameId }, fulfill, reject, timeout);
            });
        },
        showGameDetailsUI: function (userId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_GAMEDETAILS, 'Show': true}, fulfill, reject, timeout);
            });
        },
        showBroadcastUI: function (userId, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_IGO, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_BROADCAST, 'Show': true}, fulfill, reject, timeout);
            });
        },
        showStoreUI: function (userId, categories, masterTitleIds, offerIds, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_COMMERCE, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_STORE, 'Show': true, 'Categories': convertToArray(categories), 'MasterTitleIds': convertToArray(masterTitleIds), 'Offers': convertToArray(offerIds) }, fulfill, reject, timeout);
            });
        },
        showCheckoutUI: function (userId, offerIds, timeout) {
            return new Promise(function (fulfill, reject) {
                sendRequest(FACILITY_COMMERCE, COMMAND_SHOWIGOWINDOW, { 'UserId': userId, 'WindowId': IGOWINDOW_CHECKOUT, 'Show': true, 'Offers': convertToArray(offerIds) }, fulfill, reject, timeout);
            });
        },

        // Generated Functions.
"

    foreach($i in $Schema.ComplexTypes)
    {
        if($i.facility -ne $null)
        {
            $args = "";
            $init = "";
            foreach($a in $i.Attributes)
            {
                if($args.Length -gt 0)
                {
                    $args += ", " + $a.Name;
                    $init += ", '" + $a.Name + "': " + $a.Name; 
                }
                else
                {
                    $args = $a.Name;
                    $init += "'" + $a.Name + "': " + $a.Name; 
                }
            }
            foreach($e in $i.Elements)
            {
                if($args.Length -gt 0)
                {
                    $args += ", " + $e.Name;
                    if ($e.IsArray -eq $true)
                    {
                        $init += ", '" + $e.Name + "': convertToArray(" + $e.Name + ")"; 
                    }
                    else
                    {
                        $init += ", '" + $e.Name + "': " + $e.Name; 
                    }
                }
                else
                {
                    $args = $e.Name;
                    $init += "'" + $a.Name + "': " + $a.Name; 
                }
            }
            if($args.Length -gt 0)
            {
                $args += ", timeout";
            }
            else
            {
                $args = "timeout";
            }

            if($i.Documentation.Length -gt 0)
            {
       "        // " + $i.Documentation
            }

       "        " + $i.Name.SubString(0, 1).ToLower() + $i.Name.SubString(1) + ": function (" + $args + ") {"
       "            return new Promise(function (fulfill, reject) {"
       "                sendRequest(FACILITY_" + $i.Facility.ToUpper() + ", COMMAND_" + $i.Name.ToUpper() + ", { " + $init + " }, fulfill, reject, timeout);" 
       "            });"
       "        },"
        }
    }

    foreach($i in $Schema.ComplexTypes)
    {
        if($i.isEvent -eq $true)
        {
        "        register" + $i.Name + ": function (eventFunc) {"
        "            registerEvent(EVENT_" + $i.Name + ", eventFunc);"
        "        },"
        }
    }

"        registerEventFallback: function (func) {
            eventFallback = func;
        }
    };
}());"

