rds = Redis:new()
redis = {}

function redis.connect(ip, port)
    return rds:connect(ip, port)
end
function redis.getString(key)
    return rds:getString(key)
end

function redis.setString(key, strValue)
    return rds:setString(key, strValue)
end


return redis
--rds = Redis:new()
