#////////////////////////////////////////////////////////
#/// FRIEND SERVICE

#////////////////////////////////////////

# Invite 2387045441 From 2385184937
# **FriendServiceClient::inviteFriend(2387045441, "TEST") // jrivero@rocketmail.com
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJxkMdIf8KQXpEgonezCKKk6i8OmuXfIPPVWnTxTW2KR4V_U7evLotZHvvfWrVB6JM4BWExFxjv3iP1fswmfuGK" --insecure -i "https://friends.dm.origin.com/friends/inviteFriend?nucleusId=2385184937&friendId=2387045441&comment=TEST" > "friends/inviteFriend;comment=TEST;friendId=2387045441;nucleusId=2385184937"
#---------

# Reject 2385184937 From  2387045441
# **FriendServiceClient::rejectInvitation(2385184937);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLLnQWgenwpMA1XcXkWZnARsfaDxlzy6-zgnbXvcwoniCdgr8jTab-bbJFdH0R4cTUt6EPVkzDeXTJJ8Gz78msTr" --insecure -i "https://friends.dm.origin.com/friends/rejectInvitation?nucleusId=2387045441&friendId=2385184937" > "friends/rejectInvitation;friendId=2385184937;nucleusId=2387045441"
#---------

# Invite 2385184937 from 2387213056
# **FriendServiceClient::inviteFriend(2385184937, "TEST")
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLLTBC7sj7nBocfGm8UqgIInXD2XZN2MYtFYhIlBHpBafNP8QjncbShXN3-iaogfkCv363L_ikJcS_TOLmbtlPMA" --insecure -i "https://friends.dm.origin.com/friends/inviteFriend?nucleusId=2387213056&friendId=2385184937&comment=TEST" > "friends/inviteFriend;comment=TEST;friendId=2385184937;nucleusId=2387213056"
#---------
# From 2385184937 Confirm 2387213056
# **FriendServiceClient::confirmInvitation(2385184937);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLL0qWlxd13XDrk_oHu3f1bcDniPcGzAmsBjT8NDZhVft_Nai5IBQ9mxYK02en3Yw8BKpckHWlrEUgET1F56FML-" --insecure -i "https://friends.dm.origin.com/friends/confirmInvitation?nucleusId=2385184937&friendId=2387213056" > "friends/confirmInvitation;friendId=2387213056;nucleusId=2385184937"
#---------

#-----------------------------------------------------------------------------------------------------
# **FriendServiceClient::inviteFriend(123456, "TEST") NON EXISTENT
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJxkMdIf8KQXpEgonezCKKk6i8OmuXfIPPVWnTxTW2KR4V_U7evLotZHvvfWrVB6JM4BWExFxjv3iP1fswmfuGK" --insecure -i "https://friends.dm.origin.com/friends/inviteFriend?nucleusId=2385184937&friendId=123456&comment=TEST" > "friends/inviteFriend;comment=TEST;friendId=123456;nucleusId=2385184937"

# From 2387213056 retrieve pending friends
# **FriendServiceClient::retrievePendingFriends();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJJPyutn1MttKctebaYrGB4gitaLPmlMiYxLHL42un-RYeaS3ndytX4zqlkllIWLmd4Fx9ZUZ_nKZyPiVcnYKLr" --insecure -i "https://friends.dm.origin.com/friends/user/2387213056/pendingFriends?nucleusId=2387213056" > "friends/user/2387213056/pendingFriends;nucleusId=2387213056"


#---------
# **FriendServiceClient::inviteFriendByEmail("sr.jrivero@gmail.com");
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJxkMdIf8KQXpEgonezCKKk6i8OmuXfIPPVWnTxTW2KR4V_U7evLotZHvvfWrVB6JM4BWExFxjv3iP1fswmfuGK" --insecure -i "https://friends.dm.origin.com/friends/inviteFriendByEmail?nucleusId=2385184937&email=sr.jrivero@gmail.com" > "friends/inviteFriendByEmail;email=sr.jrivero@gmail.com;nucleusId=2385184937"

#---------
# **FriendServiceClient::retrieveInvitations();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJxkMdIf8KQXpEgonezCKKk6i8OmuXfIPPVWnTxTW2KR4V_U7evLotZHvvfWrVB6JM4BWExFxjv3iP1fswmfuGK" --insecure -i "https://friends.dm.origin.com/friends/retrieveInvitation?nucleusId=2385184937" > "friends/retrieveInvitation;nucleusId=2385184937"

curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLK4O9Sj2sa0Jz5GQssHBsl7DyVDeJQqGT1EcGmQDy9YUBti-nyqKik2K8oaWDyDU7_XmsuuVCBaMZR1f7vpwv0S" --insecure -i "https://friends.dm.origin.com/friends/retrieveInvitation?nucleusId=2387213056" > "friends/retrieveInvitation;nucleusId=2387213056"


# **FriendServiceClient::deleteFriend(2387213056);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJr4dxkyc6U-gFdHtm0n3d1qRZjVktNwCP4lxvMC27p3rob-eyFmEwg63vjQKngX255WmkOzHYO7SYo6s-vpD6Q" --insecure -i "https://friends.dm.origin.com/friends/deleteFriend?nucleusId=2385184937&friendId=2387213056" > "friends/deleteFriend;friendId=2387213056;nucleusId=2385184937"

#---------
# **FriendServiceClient::blockUser(2387213056);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLLXIi2Kquj2PBvouykKhwrdtj_XBezvGu72GXrStt71AxyNfSl_NwSx98vAdFBvvpXOY_tFWKcL-EyuBNH9YJz-" --insecure -i "https://friends.dm.origin.com/friends/blockUser?nucleusId=2385184937&friendId=2387213056" > "friends/blockUser;friendId=2387213056;nucleusId=2385184937"

#---------
# **FriendServiceClient::unblockUser(2387213056);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLLtxvUOQF7zNRnpuBPyUC095xgoiayvOMaJ-ImNGyySckBGOnIEKymaR-yzxOVQzQ4FRGzABVmaDnXG7jM1XU4A" --insecure -i "https://friends.dm.origin.com/friends/unblockUser?nucleusId=2385184937&friendId=2387213056" > "friends/unblockUser;friendId=2387213056;nucleusId=2385184937"

#---------
# **FriendServiceClient::blockedUsers();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLLXIi2Kquj2PBvouykKhwrdtj_XBezvGu72GXrStt71AxyNfSl_NwSx98vAdFBvvpXOY_tFWKcL-EyuBNH9YJz-" --insecure -i "https://friends.dm.origin.com/friends/getBlockUserList?nucleusId=2385184937" > "friends/getBlockUserList;nucleusId=2385184937"

#---------
# **FriendServiceClient::healthCheck()
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJxkMdIf8KQXpEgonezCKKk6i8OmuXfIPPVWnTxTW2KR4V_U7evLotZHvvfWrVB6JM4BWExFxjv3iP1fswmfuGK" --insecure -i "https://friends.dm.origin.com/friends/health_check" > "friends/health_check"
#---------
