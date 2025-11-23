-- init


package.path=package.path..';./script/?.lua'
redis =require("redis")
local ret=redis.connect("localhost",6379)

redis.setString("a_key2_3","value——1111")

local value = redis.getString("a_key2_3")
print("vvvvvvvvvv = ".. value)


event_init()

-- create server
server = NetServer:new()
server.on_accept = function(conn)
	print("new connected")
	local pack = NetPacket:new()
	pack:pushString("msg 123456")
	conn:sendPacket(1, pack)
end

server.on_close = function(conn)
	print("closed......")
end

server.on_msg = function(conn, msgtype, pack)
	local str = pack:getString()
	print("body=",str,  "msgId =", msgtype)
	conn:sendPacket(1, pack)
end

server:listen("0.0.0.0", 3002, false)

event_run()