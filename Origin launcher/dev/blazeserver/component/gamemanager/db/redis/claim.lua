-- Brief: This script is designed to maintain a sequential mapping of worker ids
-- d:\dev\gs2\blazeserver\bin\redis\win64>redis-cli.exe --eval "d:\dev\gs2\blazeserver\etc\component\matchmaker\claim.lua" 455 10
if #KEYS < 1 or #ARGV < 1 then
    -- redis.log(redis.LOG_WARNING,"Something is wrong with this script, keys:" .. #KEYS .. " argv: " .. #ARGV)
    return redis.error_reply("Required arguments: keyPrefix,incomingWorkerId[,TTL]");
end;
local keyPrefix = KEYS[1];
local workerKeyPrefix = keyPrefix .. '.id.';
local versionKey = keyPrefix .. '.version';
local timeToLiveSeconds = 1000; -- TTL defaults to 1000 seconds
if #ARGV == 2 then
    timeToLiveSeconds = ARGV[2];
end;
local incomingWorker = ARGV[1];
local incomingWorkerKey = workerKeyPrefix .. incomingWorker;
local workerKeys = redis.call('KEYS', workerKeyPrefix .. '*');
local workerValues = {};
if #workerKeys > 0 then
    workerValues = redis.call('MGET', unpack(workerKeys));
end;
local incomingWorkerSlot = 0;
local workersBySlot = {}; -- unordered map of workers keyed by numeric slot
local workerSlots = {}; -- array of numeric slots

for i, workerValue in ipairs(workerValues) do 
    local workerKey = workerKeys[i];
    local slot = tonumber(workerValue); 
    if incomingWorkerKey == workerKey then 
        incomingWorkerSlot = slot; -- incoming worker already has slot
    end; 
    workersBySlot[slot] = workerKey;
    table.insert(workerSlots, slot);
    --print('worker:', workerKey, 'slot:', slot);
end;

table.sort(workerSlots);

local previousWorkerSlot = incomingWorkerSlot; -- save previously assigned slot value
if incomingWorkerSlot == 0 then 
    if #workerSlots > 0 then 
        incomingWorkerSlot = workerSlots[#workerSlots] + 1; -- insert incoming item at an artificial slot, so that it's first to move
    else
        incomingWorkerSlot = 1; -- assign to first slot since the table is empty
    end;
    table.insert(workerSlots, incomingWorkerSlot); -- ordered invariant is maintained
    workersBySlot[incomingWorkerSlot] = incomingWorkerKey;
    --print('incoming worker : ' .. incomingWorkerKey .. ' not found, inserting into temporary slot : ' .. incomingWorkerSlot);
else
    --print('incoming worker : ' .. incomingWorkerKey .. ' found at slot : ' .. incomingWorkerSlot);
end;

if incomingWorkerSlot > #workerKeys then
    --print('incoming worker slot : ' .. incomingWorkerSlot .. ' is outside the range : 1..' .. #workerKeys);
    
    for slot = 1, #workerSlots do
        local value = workersBySlot[slot];
        if value ~= nil then 
            if incomingWorkerSlot == slot then
                --print('test slot : ' .. slot .. ' occupied by *incoming* worker :  .. value);
                break;
            else
                --print('test slot : ' .. slot .. ' occupied by *existing* worker : ' .. value);
            end;
        else
            -- backfill empty slots in the workers by successively popping from workerSlots
            local lastWorkerSlot = table.remove(workerSlots);
            if incomingWorkerSlot == lastWorkerSlot then
                --print('test slot : ' .. slot .. ' is free, replacing with *incoming* worker from slot : ' .. lastWorkerSlot .. ', value : ' .. workersBySlot[lastWorkerSlot]);
                incomingWorkerSlot = slot; -- incoming worker was assigned to an empty slot
                break;
            else
                --print('test slot : ' .. slot .. ' is free, replacing with *existing* worker from slot : ' .. lastWorkerSlot .. ', value : ' .. workersBySlot[lastWorkerSlot]);
            end;
        end;
    end;
else
    -- incoming value is within the range of assigned slots, no change necessary
    --print('incoming worker slot : ' .. incomingWorkerSlot .. ' is inside the the range : 1..' .. #workerKeys .. ', we are done.');
end;

local version = 0;
if previousWorkerSlot == incomingWorkerSlot then
    redis.call('EXPIRE', incomingWorkerKey, timeToLiveSeconds); -- refresh worker key ttl
    version = tonumber(redis.call('GET', versionKey)); -- get version
else
    redis.call('SETEX', incomingWorkerKey, timeToLiveSeconds, incomingWorkerSlot); -- insert worker key with ttl
    version = redis.call('INCR', versionKey); -- increment version
end;

return { incomingWorkerSlot, previousWorkerSlot, version, #workerSlots };