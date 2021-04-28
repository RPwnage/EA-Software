--[[
  Exchanges an instance name for an InstanceId (implemented as a Lua script for atomicity).
  
  Arguments:
   [1] instance name
   [2] current instance id
   [3] TTL of instance id allocation
   [4] maximum valid instance id

   Note that redis maintains two maps for instance id allocation:
   (1) a map of instance ids by instance name, in which the entries expire after the TTL (Example:<coreSlave1, 42> : 2m, <coreSlave2, 52>: 2m).
   (2) a map of instance names by instance id, in which the entries do not expire (Example: <42, coreSlave1>, <52, coreSlave2>)

   The map of instance names by instance id provides a quick way to identify which instance ids may be in use when the instance id counter rolls over.
   It also allows the script to refresh instance id allocations for Blaze instances that obtained an initial instance id, but then lost connectivity 
   to the instance id allocator while the TTL expired. This ensures that the instance id associated with a given instance name can only change after
   the instance is restarted, or a new instance is being assigned an instance id.

   A field-map of all known instance names is also maintained. (Redis field-maps do not support expiration by TTL.)
   This provides a way to get all current (and previous) instance names without using SCAN or KEYS.
   Since the fields do not expire, the field-map may contain instance names that are no longer in use.
   As such, other data sources (such as the map of instance ids by instance name) should be used to validate the instance's existence before use.
   A dummy value of "1" is stored for the field to emphasize that the other map should be used to retrieve the instance id, if needed.
--]]

local keyPrefix = '{' .. KEYS[1] .. '}.'

local instanceName = ARGV[1]
local expectedId = ARGV[2]
local allocationTtl = ARGV[3]
local instanceIdMax = ARGV[4]

local instanceIdsByName = keyPrefix .. 'InstanceIdsByName.' .. instanceName

local currentId = redis.call('GET', instanceIdsByName)
local isExpired = (not currentId)

-- If the value is not in expiring map but instance thinks it has a legit value.
if (isExpired and (tonumber(expectedId) ~= 0)) then
    -- If the unexpiring map knows that this instance was around and had the id it thinks it has, give this guy back his id.
    local currentName = redis.call('GET', keyPrefix .. 'InstanceNamesById.' .. expectedId)
    if (currentName == instanceName) then
        currentId = expectedId
    end
end

-- If the instance has succeeded in getting id by expiring or unexpiring map, add its new expiry and return.
if (currentId) then
    if (isExpired) then
        redis.call('SET', instanceIdsByName, currentId, 'EX', allocationTtl)

        -- Add this instance to the InstanceNamesLedger
        redis.call('INCR', keyPrefix .. 'InstanceNamesLedger.VersionInfo')
        redis.call('HSET', keyPrefix .. 'InstanceNamesLedger', instanceName, '1')
    else
        redis.call('EXPIRE', instanceIdsByName, allocationTtl)
    end
    return tonumber(currentId)
end

-- At this point, we took care of the running instance that might have suffered network partitioning or was executing happy path.
-- Now, we have a situation where we need to allocate a new id using our counter key for a newly started instance.
local counter = keyPrefix .. 'InstanceIdCounter'
local newId = nil
local nextId = redis.call('INCR', counter)

-- If overflowing, let's roll over the counter to 1.
if (tonumber(nextId) > tonumber(instanceIdMax)) then
    redis.call('SET', counter, '1')
    nextId = 1
end

-- We found a starting value based on the counter, repeat until we can make sure that no one else has the id.
local startid = tonumber(nextId)
repeat

    -- check the id in the unexpiring map
    local existingInstanceName = redis.call('GET', keyPrefix .. 'InstanceNamesById.' .. nextId)

    if (not existingInstanceName) then
        -- no one is using the id, so give it to the guy requesting it.
        newId = nextId
    else
        -- an instance previously used this id is it still being used by that instance?
        local idInUse = redis.call('GET', keyPrefix .. 'InstanceIdsByName.' .. existingInstanceName)
        if (tonumber(nextId) ~= tonumber(idInUse)) then
            -- Not being used by other instance name, so reuse the id.
            newId = nextId
        else
            -- Try the next id.
            nextId = redis.call('INCR', counter)

            -- If overflowing, let's roll over the counter to 1.
            if (tonumber(nextId) > tonumber(instanceIdMax)) then
                redis.call('SET', counter, '1')
                nextId = 1
            end
        end
    end

-- if we have reached our startId, we have made a loop and exhausted ourselves. Bail out.
until (newId ~= nil or tonumber(nextId) == startid)

if (newId == nil) then
    return redis.error_reply('No available instance ids')
end

redis.call('SET', keyPrefix .. 'InstanceNamesById.' .. newId, instanceName)
redis.call('SET', instanceIdsByName, newId, 'EX', allocationTtl)

-- Add this instance to the InstanceNamesLedger
redis.call('INCR', keyPrefix .. 'InstanceNamesLedger.VersionInfo')
redis.call('HSET', keyPrefix .. 'InstanceNamesLedger', instanceName, '1')

return tonumber(newId)
