{
    \"name\": \"$testname\",
    \"inventoryId\": $invId,
    \"configId\": $configId,
    \"containerImage\": \"$clientrepo:$servicename\",
    \"maxLoadPerProcess\": $numConns,
    \"extraParameters\": \"$params\",
    \"loggingEnabled\": true,
    \"consoleLoggingEnabled\": true,
    \"loadPattern\": [
      {
        \"startIndex\": $startIndex,
        \"rampTimeInSecs\": $rampupSecs,
        \"offsetInMins\": 0,
        \"loadChange\": $load
      }
    ],
    \"durationInMins\": $durationMins,
    \"terminateEnv\": $cleanupClientNodes,
    \"autoGenerateReport\": true
}
