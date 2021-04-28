
function people_get_response()
{
    var resp = {
        "xuid": "1",
        "isFavorite": false,
        "isFollowingCaller": false,
        "socialNetworks": ["LegacyXboxLive"]
    }

    return resp;
}

function scids_get_response()
{
    var resp = {
        "user": {
            "xuid": "123456789",
            "gamertag": "WarriorSaint",
            "stats": [
                {
                    "statname": "Wins",
                    "type": "Integer",
                    "value": 40
                },
                {
                    "statname": "Kills",
                    "type": "Integer",
                    "value": 700
                },
                {
                    "statname": "KDRatio",
                    "type": "Double",
                    "value": 2.23
                },
                {
                    "statname": "Headshots",
                    "type": "Integer",
                    "value": 173
                }
            ],
        }
    }
    return resp;
}

function profile_post_response()
{
    var resp = {
        "profileUsers":[
            {
                "id":"2533274791381930",
                "settings":[
                {
                  "id":"GameDisplayName",
                  "value":"John Smith"
                },
                {    
                  "id":"GameDisplayPicRaw",
                  "value":"http://images-eds.xboxlive.com/image?url=z951ykn43p4FqWbbFvR2Ec.8vbDhj8G2Xe7JngaTToBrrCmIEEXHC9UNrdJ6P7KIN0gxC2r1YECCd3mf2w1FDdmFCpSokJWa2z7xtVrlzOyVSc6pPRdWEXmYtpS2xE4F"
                },
                {
                  "id":"Gamerscore",
                  "value":"0"
                },
                {
                  "id":"Gamertag",
                  "value":"CracklierJewel9"
                }
                ]
            }
        ]
    }
    return resp;
}

function empty_post_response()
{
    var resp = {};
    return resp;
}

module.exports.profile_post_response = profile_post_response;
module.exports.empty_post_response = empty_post_response;
module.exports.people_get_response = people_get_response;
module.exports.scids_get_response = scids_get_response;
