#/bin/bash 

# tester for fetching blazeserver status variables

if [ ${#@} -ne 4 ]; then
  echo "Usage: $0 TargetPSU_in_thousands service_name environment platform"
  exit -1
fi

targetpsu=$1
let fulltargetpsu=$targetpsu*1000
let targetDeviation=10
let targetGMGauge_ACTIVE_GAMES=94381*$targetpsu/750
let targetGMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED=5236*$targetpsu/750
let targetGMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH=113444*$targetpsu/750
let targetGMGauge_ACTIVE_PLAYERS=253000*$targetpsu/750
let targetGMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED=29000*$targetpsu/750
let targetGMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH=225000*$targetpsu/750
let targetSSGaugeGameLists=5645*$targetpsu/750
let targetMMGaugeActiveMatchmakingSession=1000

servicename=$2
environment=$3
platform=$4

./fsv $servicename $environment $platform coreMaster \
  --component gamemanager_master \
    +GMGauge_ACTIVE_GAMES%$targetDeviation-$targetGMGauge_ACTIVE_GAMES \
    +GMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED%$targetDeviation-$targetGMGauge_ACTIVE_GAMES_CLIENT_SERVER_DEDICATED \
    +GMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH%$targetDeviation-$targetGMGauge_ACTIVE_GAMES_PEER_TO_PEER_FULL_MESH \
    +GMGauge_ACTIVE_PLAYERS%$targetDeviation-$targetGMGauge_ACTIVE_PLAYERS \
    +GMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED%$targetDeviation-$targetGMGauge_ACTIVE_PLAYERS_CLIENT_SERVER_DEDICATED \
    +GMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH%$targetDeviation-$targetGMGauge_ACTIVE_PLAYERS_PEER_TO_PEER_FULL_MESH \
 --component usersessions \
    GaugeUS_CLIENT_TYPE_GAMEPLAY_USER%$targetDeviation-$fulltargetpsu 

echo

./fsv $servicename $environment $platform searchSlave \
  --component search \
    +SSGaugeGameLists%$targetDeviation-$targetSSGaugeGameLists \
    +SSGaugeActiveFGSessions 

echo

./fsv $servicename $environment $platform mmSlave \
  --component matchmaker \
    MMGaugeActiveMatchmakingSession+$targetMMGaugeActiveMatchmakingSession \
    +ScenarioMMTotalMatchmakingSessionStarted \
    +ScenarioMMTotalMatchmakingSessionSuccess \
    +ScenarioMMTotalMatchmakingSuccessDurationMs \
    +ScenarioMMTotalMatchmakingSessionsCreatedNewGames \
    +ScenarioMMTotalMatchmakingSessionsJoinedNewGames \
    +ScenarioMMTotalMatchmakingSessionsJoinedExistingGames \
    +ScenarioMMTotalMatchmakingSessionTimeout \
  --component usersessions \
    GaugeUS_CLIENT_TYPE_GAMEPLAY_USER%$targetDeviation

echo

./fsv $servicename $environment $platform mmSlave \
  --component matchmaker \
    MMGaugeActiveMatchmakingSession+$targetMMGaugeActiveMatchmakingSession \
    +MMTotalMatchmakingSessionStarted \
    +MMTotalMatchmakingSessionSuccess \
    +MMTotalMatchmakingSessionsCreatedNewGames \
    +MMTotalMatchmakingSessionsJoinedExistingGames \
    +MMTotalMatchmakingSessionTimeout \
  --component usersessions \
    GaugeUS_CLIENT_TYPE_GAMEPLAY_USER%$targetDeviation

