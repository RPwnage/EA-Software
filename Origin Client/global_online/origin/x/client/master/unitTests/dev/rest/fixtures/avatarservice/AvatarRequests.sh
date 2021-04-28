#////////////////////////////////////////////////////////
#/// AVATAR 
# **AvatarServiceClient::allAvatarTypes();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/avatartypes" > "avatar/avatartypes"

#---------
# **AvatarServiceClient::existingAvatarType();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/avatartype/1" > "avatar/avatartype/1"

#---------
# **AvatarServiceClient::userAvatarChanged(AT_NORMAL);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/user/2385184937/avatar/1/ischanged" > "avatar/user/2385184937/avatar/1/ischanged"

#---------
# **AvatarServiceClient::avatarById(AT_NORMAL);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/avatarinfo/1" > "avatar/avatarinfo/1"

#---------
# **AvatarServiceClient::avatarsByGalleryId(1);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/gallery/1/avatars" > "avatar/gallery/1/avatars"

#---------
# **AvatarServiceClient::avatarsByUserIds(list);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/user/2387213056;2385184937/avatars" > "avatar/user/2387213056;2385184937/avatars"

#---------
# **AvatarServiceClient::defaultAvatar();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/defaultavatar" > "avatar/defaultavatar"

#---------
# **AvatarServiceClient::recentAvatarList();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/recentavatars" > "avatar/recentavatars"

#---------
# **AvatarServiceClient::supportedDimensions();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLJgfH_jWYUpj5q2nOIBWJL1QvtxsPZAJ_fpC3ZrYdb4tX1f0N2ONhXLCwVEDu_8uVhbuLSKCHIG02wzMKFgwW_5" --insecure -i "https://avatar.dm.origin.com/avatar/dimensions" > "avatar/dimensions"

#---------
