function batch_post_response()
{
    var resp = { 
        "users":[          
        {    
            "xuid": "123456789",
            "gamertag": "WarriorSaint",
            "scids":[
            {
                "scid":"c402ff50-3e76-11e2-a25f-0800200c1212",
                "stats":  [
                {
                 "statname":"Test4FirefightKills",
                 "type":"Integer",
                 "value":7
                },
                {
                 "statname":"Test4FirefightHeadshots",
                 "type":"Integer",
                 "value":4
                }]
            },
            {
                "scid":"c402ff50-3e76-11e2-a25f-0800200c0343",
                "stats":  [
                {
                 "statname":"OverallTestKills",
                 "type":"Integer",
                 "value":3434
                },
                {
                 "statname":"TestHeadshots",
                 "type":"Integer",
                 "value":41
                }]
            }],
        },
        {    
            "gamertag":"TigerShark",
            "xuid":1234567890123234,
            "scids":[
            {
             "scid":"123456789",
             "stats":  [
                {
                 "statname":"Test4FirefightKills",
                 "type":"Integer",
                 "value":63
                },
                {
                 "statname":"Test4FirefightHeadshots",
                 "type":"Integer",
                 "value":12
                }]
            },
            {
             "scid":"987654321",
             "stats":  [
                {
                 "statname":"OverallTestKills",
                 "type":"Integer",
                 "value":375
                },
                {
                 "statname":"TestHeadshots",
                 "type":"Integer",
                 "value":34
                }]
            }],
        }]
    }
    return resp;
}

module.exports.batch_post_response = batch_post_response;
