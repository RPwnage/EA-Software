$SchemaFilename = $Generator.NamespaceToFilename($Schema.Namespace).Replace('/','.')
$SchemaFilename + "_info.js"
"if (typeof(Origin) === 'undefined') {
    Origin = {};
}
if(typeof(Origin.SDK) === 'undefined') {
    Origin.SDK = {};
}
Origin.SDK.UI = [{
    ""functionName"": ""startup"",
    ""groups"": [""Startup""],
    ""documentation"": ""This function initializes the SDK, and should be called before any other functions"",
    ""arguments"":{
        ""contentId"": {
            ""type"": ""string"",
            ""type_detail"": ""string"",
            ""documentation"": ""The contentId identifies your application for Origin."",
            ""default"":""71314""
        },
        ""title"": {    
            ""type"": ""string"",
            ""type_detail"": ""string"",
            ""documentation"": ""The title is used in cases where the contentId doesn't match any of the users entitlements."",
            ""default"": ""Origin SDK (JS) Test Page.""
        },
        ""multiplayerId"": {
            ""type"": ""string"",
            ""type_detail"": ""string"",
            ""documentation"": ""The multiplayerId identifies all games that can play together, and is usually set to the contentId of the base game."",
            ""default"" : ""71314""
        },
        ""language"": {
            ""type"": ""string"",
            ""type_detail"": ""string"",
            ""documentation"": ""Currently not used, but will be used in the future to have certain IGO windows in the proper language."",
            ""default"": ""en_US""
        },
        ""connectionId"": {
            ""type"": ""string"",
            ""type_detail"": ""string"",
            ""documentation"": ""An id passed into the application that hosts the browser, by the Origin SDK(C++) or through the environment."",
            ""default"": ""{84B70730-5FBC-4ab0-A284-070D2D6DC365}""
        }
    },
    ""return"": ""promise"",
    ""successCode"":""Origin.SDK.Support.originIsConnected();"",
    ""failureCode"":""Origin.SDK.Support.originIsDisconnected();""
},
{
    ""functionName"": ""shutdown"",
    ""groups"": [""Startup""],
    ""documentation"": ""This function shutsdown the SDK, after this the connection with Origin is terminated."",
    ""arguments"":{},
    ""return"": ""void"",
    ""successCode"":""Origin.SDK.Support.originIsDisconnected();""
},
{
    ""functionName"": ""getDefaultUser"",
    ""groups"": [""User""],
    ""documentation"": ""This function returns the logged in users userId."",
    ""arguments"":{},
    ""return"": ""string""
},
{
    ""functionName"": ""getDefaultPersona"",
    ""groups"": [""User""],
    ""documentation"": ""This function returns the logged in users personaId."",
    ""arguments"":{},
    ""return"": ""string""
},
"
foreach($i in $Schema.ComplexTypes)
{
    if($i.facility -ne $null)
    {
"{
    ""functionName"": """ + $i.Name.SubString(0, 1).ToLower() + $i.Name.SubString(1) + ""","

        if($i.Documentation.Length -ne 0)
        {
"    ""documentation"": """ + $i.Documentation + ""","
        }
"    ""groups"":[""" + $i.Groups + """],";
"    ""arguments"": {"
        foreach($a in $i.Attributes)
        {
"        """ + $a.Name.SubString(0, 1).ToLower() + $a.Name.SubString(1) + """: {"
            if($a.TypeST -ne $null)
            {
"            ""type"": ""enum"","
"            ""values"":["
                $st = $a.TypeST;
                for($e = 0; $e -lt $st.Enumerations.Count; $e++)
			    {
"                {"
                    if($st.EnumerationDocumentation[$e].Length -gt 0)
                    {
"                    ""value"": """ + $st.Name.ToUpper() + "_" + $st.Enumerations[$e] + ""","
"                    ""documentation"": """ + $st.EnumerationDocumentation[$e] + """"
                    }
                    else
                    {
"                    ""value"": """ + $st.Name.ToUpper() + "_" + $st.Enumerations[$e] + """"
                    }
"                },"
			    }
"            ],"
            }
            else
            {
"            ""type"": ""string"","
            }
"            ""type_detail"": """ + $a.TypeName + ""","
            if($a.Documentation.Length -ne 0)
            {
"            ""documentation"": """ + $a.Documentation + """"
            }
"        },"
        }
        foreach($e in $i.Elements)
        {
"        """ + $e.Name.SubString(0, 1).ToLower() + $e.Name.SubString(1) + """: {"
            if($e.IsArray)
            {
"            ""type"": ""array"","
            }
            else
            {
"            ""type"": ""string"","
            }
"            ""type_detail"": """ + $e.TypeName + ""","
            if($e.Documentation.Length -ne 0)
            {
"            ""documentation"": """ + $e.Documentation + """"
            }
"        },"
        }
"        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }"
"    },"
"    ""return"": ""promise"""
"},"
    }
}
foreach($i in $Schema.ComplexTypes)
{
    if($i.isEvent -eq $true)
    {
"{"
"    ""functionName"": ""register" + $i.Name + ""","
"    ""groups"":[""" + $i.Groups + """],";
"    ""documentation"": ""Register a callback for the " + $i.Name + " event."","
"    ""arguments"": {"
"        ""eventFunc"": {"
"            ""type"": ""function"""
"        }"
"    },"
"    ""return"": ""callback"""
"},"
    }
}
"{"
"    ""functionName"": ""registerEventFallback"","
"    ""groups"":[""Events""],"
"    ""documentation"": ""Register a callback for any event that is not registering a callback."","
"    ""arguments"": {"
"        ""eventFunc"": {"
"            ""type"": ""function"""
"        }"
"    },"
"    ""return"": ""callback"""
"},
{
    ""functionName"": ""showProfileUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the current users profile."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showFriendsProfileUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the profile of another user."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""friendId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the user you want to see the profile for.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showBrowserUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the IGO Browser."",
    ""arguments"":{
        ""flags"": {
            ""type"": ""string"",
            ""type_detail"": ""int"",
            ""documentation"": ""Bitfield of SHOW_NAV(1), IN_TAB(2), ACTIVE(4)""
        },
        ""url"": {
            ""type"": ""string"",
            ""type_detail"": ""string"",
            ""documentation"": ""The URL to navigate to.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showFriendsUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the Friends list."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showFindFriendsUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the Find Friends UI."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showChangeAvatarUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Allow the user to change his avatar."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showRequestFriendUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Allow the user to send a friend request through the IGO."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""friendId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The user to befriend.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showcomposeChatUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Allow the user to compose a chat message."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""friendId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The user to chat with.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showInviteUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the game invite UI, where the user can select the friend to play width."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""friendIds"": {
            ""type"": ""array"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The list of users to play with.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showAchievementUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the users achievements."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""personaId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The personaId of the user you want to see the achievements for.""
        },
        ""gameId"": {
            ""type"": ""string"",
            ""documentation"": ""Optional The achievementSet Id of the game to show achievements for. if undefined show the current game.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showGameDetailsUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the current game details in the IGO."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
}, 
{
    ""functionName"": ""showBroadcastUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the Twitch settings/controls in the IGO."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showStoreUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the addon store in the IGO."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""categories"": {
            ""type"": ""array"",
            ""type_detail"": ""string"",
            ""documentation"": ""An array of categories to get the offers from. (not implemented leave empty).""
        },
        ""masterTitleIds"": {
            ""type"": ""array"",
            ""type_detail"": ""string"",
            ""documentation"": ""An array of masterTitle Ids to get the offers from.""
        },
        ""offerIds"": {
            ""type"": ""array"",
            ""type_detail"": ""string"",
            ""documentation"": ""An array of specific offers.""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
},
{
    ""functionName"": ""showCheckoutUI"",
    ""groups"":[""IGO""],
    ""documentation"": ""Show the Lockbox Checkout UI."",
    ""arguments"":{
        ""userId"": {
            ""type"": ""string"",
            ""type_detail"": ""unsignedLong"",
            ""documentation"": ""The userId of the current user.""
        },
        ""offerIds"": {
            ""type"": ""array"",
            ""type_detail"": ""string"",
            ""documentation"": ""An array of offers to buy. (Currently only supports one offer at a time.)""
        },
        ""timeout"": {
            ""type"": ""string"",
            ""documentation"": ""The time to wait for a response from Origin in milliseconds."",
            ""default"": ""15000""
        }
    },
    ""return"": ""promise""
}"
"]"
