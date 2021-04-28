#////////////////////////////////////////////////////////
#/// LOGIN REGISTRATION
# **LoginRegistrationClient::userIdByEmail();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/user/email/sr.jrivero@gmail.com" > "loginregistration/user/email/sr.jrivero@gmail.com"

#---------
# **LoginRegistrationClient::userIdByEAID();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/user/eaid/zumba_el_gato" > "loginregistration/user/eaid/zumba_el_gato"

#---------
# **privacySetting(PS_ALL);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/user/2385184937/privacysettings/category/ALL" > "loginregistration/user/2385184937/privacysettings/category/ALL"

#---------
# **searchOptions();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/user/2385184937/searchoptions" > "loginregistration/user/2385184937/searchoptions"

#---------
# **updateSearchOptions();

#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/user/2385184937/searchoptions" > "loginregistration/user/2385184937/searchoptions"

#---------
# **singleSearchOption(XBOX);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/user/2385184937/singlesearchoption/XBOX" > "loginregistration/user/2385184937/singlesearchoption/XBOX"

#---------
# **checkOptionVisibility(ulist, XBOX);
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/users/2387213056;2385184937/singlesearchoption/XBOX" > "loginregistration/users/2387213056;2385184937/singlesearchoption/XBOX"

#---------
# **healthCheck();
#---------
curl -A "EA Download Manager Origin" -H "Content-Type: text/plain"  -H "AuthToken:Ciyvab0tregdVsBtboIpeChe4G6uzC1v5_-SIxmvSLIAswxKvXIigKJ84kxQTim_C3rAOk5od7HH6dxa6cBt19W2Qcymv3soAIA3Et5Rzqobq4gOcVHLbcLT1VTSOqgp" --insecure -i "https://loginregistration.dm.origin.com/loginregistration/health_check" > "loginregistration/health_check"
#---------
