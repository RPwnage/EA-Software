How to generate mockup data from real data
  1. Require Python 2.7 and requests
  2. cd to '<branch>/web/automation/mockup/scripts' directory.
  3. Open create_mockup_data.ini and edit target account settings.
     - Edit 'newPid' field and give a specific name of this mockup data set.
       Script also updates 'pidId' value in both pid.json and persona.json
       with this name.
       REASON: When one user connects to mockup XMPP server with its pid, XMPP
       server tries to load data/<pid>/friends.json, so pid value nust be same
       as folder name to allow XMPP to load roster correctly.
  4. Run
     >python create_mockup_data.py <output_path>
     ex.
     >python create_mockup_data.py ../test
     <output_path> should be relative path of working directory
  5. Mockup data will be generated under <output_path>/<newPid> directory.
  6. Copy <newPid> folder to <branch>/web/automation/mockup/data/data.

  Structure:
    mockup/
    |--data/
    |  |--1000123301435/
    |  |  |--avatar/
    |  |  |  |--<size>.xml
    |  |  |--feeds/
    |  |  |  |--<locale>/
    |  |  |     |--<feed>.json
    |  |  |--masterTitleId/
    |  |  |  |--<masterTitleId>.json
    |  |  |--offer/
    |  |  |  |--<offerId>/
    |  |  |     |--<locale>.json
    |  |  |--offerLMD/
       |  |  |--<offerId>.json
       |  |--auth.json
       |  |--entitlements.json
       |  |--friends.json
       |  |--atom/
       |  |  |--<masterTitleId>/
       |  |  |  |--usage.xml
       |  |  |--atomGameLastPlayed.xml
       |  |  |--atomUserInfoByUserIds.xml
       |  |  |--settings.xml
       |  |--auth.json
       |  |--blank.json
       |  |--configurloverride.json
       |  |--entitlements.json
       |  |--friends.json
       |  |--persona.json
       |  |--pid.json
       |-<pid>/
       |  |--...
       |-<pid>/
       |  |--...



How to mock friend list:
  1. Edit <branch>/web/automation/mockup/data/<pid>/friends.json to modify friend list.
  2. Subscribed friend should have this key/value:
       "subscription": "BOTH"
  3. Friend waiting your acceptence should have this key/value:
       "subscription": "NONE"
  4. Friend waiting your acceptence should have this key/value:
       "ask": "SUBSCRIBE"


How to mock friend's action:
  1. XMPP mockup server supports restful POST call to mock friend's action.
  2. URL: http://localhost:1402/xmpp/<jid>/<friend's jid>@127.0.0.1/<action>
  3. Actions:
    - sendMessage
      -- HEADER: Content-Type: application/json; charset=utf-8
      -- BODY: {"message":"<data>"}
    - sendAccept
    - sendReject
    - sendFriendRequest
    - sendFriendRequestRevoke
    - sendFriendRemove
    - sendPresence
      -- HEADER: Content-Type: application/json; charset=utf-8
      -- BODY: {"presence": "ONLINE" | "AWAY" | "OFFLINE"}