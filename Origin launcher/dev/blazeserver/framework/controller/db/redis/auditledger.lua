local keyPrefix = '{' .. KEYS[1] .. '}.'

local instanceName = ARGV[1]

local instanceIdsByName = keyPrefix .. 'InstanceIdsByName.' .. instanceName

local currentid = redis.call('GET', instanceIdsByName)
if (not currentid) then
    redis.call('INCR', keyPrefix .. 'InstanceNamesLedger.VersionInfo')
    redis.call('HDEL', keyPrefix .. 'InstanceNamesLedger', instanceName)
    currentid = 0
end

return tonumber(currentid)
