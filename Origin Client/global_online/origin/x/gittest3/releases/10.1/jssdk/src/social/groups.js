/*jshint strict: false */
/*jshint unused: false */
/*jshint undef: false */
define([
    'promise',
    'core/logger',
	'core/user',
    'core/dataManager',
    'core/urls'
], function (Promise, logger, user, dataManager, urls) {
    /** @namespace
     * @memberof Origin
     * @alias groups
     */
	 
	 return {

        /**
         * This will return a promise for the group list for the userId specified
         *
         * @param {number} userId
         * @return {promise<groupList[]>}  array of groupInfoObjects
         */
        groupListByUserId: function(userId) {
            logger.log('getGroups: in groupListByUserId(): ' + userId);
            return new Promise(function(resolve, reject) {
                            
                logger.log('getGroups: in groupListByUserId(): in new Promise');
                var endPoint = urls.endPoints.groupList;
                endPoint = endPoint.replace('{userId}', userId);
                endPoint = endPoint.replace('{pagesize}', '100');

                var config = {
                    atype: 'GET',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: true,
                    requser: false
                };

                var token = user.publicObjs.accessToken();
                if (token.length > 0) {
                    dataManager.addHeader(config, 'X-AuthToken', token);
                }

                dataManager.addHeader(config, 'X-Api-Version', '2');
                dataManager.addHeader(config, 'X-Application-Key', 'Origin');

                logger.log('getGroups: in groupListByUserId(): dataManager.enQueue:' + endPoint);
                
                var promise = dataManager.enQueue(endPoint, config, 0);
                promise.then(function(response) {
                    logger.log('getGroups: in groupListByUserId(): response:' + response);
                    resolve(response);
                }, function(error) {
                    logger.log('getGroups: in groupListByUserId(): error:' + error);
                    reject(error);
                }).catch(function(error) {
                    logger.log('getGroups: in groupListByUserId(): error:' + error);
                });
            });
        },

	 
        /**
         * This will return a promise to join the group for the groupId and userId specified
         *
         * @param {number} userId
         * @param {guid} groupId
         * @return {promise}  
         */
        acceptGroupInvite: function(groupId, userId) {
            logger.log('JSSDK groups: in acceptGroupInvite(): ' + userId);
            return new Promise(function(resolve, reject) {
                            
                logger.log('JSSDK groups: in acceptGroupInvite(): in new Promise');
                var endPoint = urls.endPoints.groupJoin;
                endPoint = endPoint.replace('{targetUserId}', userId);
                endPoint = endPoint.replace('{groupGuid}', groupId);

                var config = {
                    atype: 'POST',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: true,
                    requser: false
                };

                var token = user.publicObjs.accessToken();
                if (token.length > 0) {
                    dataManager.addHeader(config, 'X-AuthToken', token);
                }

                dataManager.addHeader(config, 'X-Api-Version', '2');
                dataManager.addHeader(config, 'X-Application-Key', 'Origin');

                logger.log('JSSDK groups: in acceptGroupInvite(): dataManager.enQueue:' + endPoint);
                
                var promise = dataManager.enQueue(endPoint, config, 0);
                promise.then(function(response) {
                    logger.log('JSSDK groups: in acceptGroupInvite(): response:' + JSON.stringify(response));
                    resolve(response);
                }, function(error) {
                    logger.log('JSSDK groups: in acceptGroupInvite(): error:' + error);
                    reject(error);
                }).catch(function(error) {
                    logger.log('JSSDK groups: in acceptGroupInvite(): error:' + error);
                });
            });
        },
		

        /**
         * This will return a promise to decline the group invite for the groupId and userId specified
         *
         * @param {number} userId
         * @param {guid} groupId
         * @return {promise}  
         */
        cancelGroupInvite: function(groupId, userId) {
            logger.log('JSSDK groups: in cancelGroupInvite(): ' + userId);
            return new Promise(function(resolve, reject) {
                            
                logger.log('JSSDK groups: in cancelGroupInvite(): in new Promise');
                var endPoint = urls.endPoints.groupInvited;
                endPoint = endPoint.replace('{targetUserId}', userId);
                endPoint = endPoint.replace('{groupGuid}', groupId);

                var config = {
                    atype: 'DELETE',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: true,
                    requser: false
                };

                var token = user.publicObjs.accessToken();
                if (token.length > 0) {
                    dataManager.addHeader(config, 'X-AuthToken', token);
                }

                dataManager.addHeader(config, 'X-Api-Version', '2');
                dataManager.addHeader(config, 'X-Application-Key', 'Origin');

                logger.log('JSSDK groups: in cancelGroupInvite(): dataManager.enQueue:' + endPoint);
                
                var promise = dataManager.enQueue(endPoint, config, 0);
                promise.then(function(response) {
                    logger.log('JSSDK groups: in cancelGroupInvite(): response:' + JSON.stringify(response));
                    resolve(response);
                }, function(error) {
                    logger.log('JSSDK groups: in cancelGroupInvite(): error:' + error);
                    reject(error);
                }).catch(function(error) {
                    logger.log('JSSDK groups: in cancelGroupInvite(): error:' + error);
                });
            });
        },
		
        /**
         * This will return a promise for the group list for the userId specified
         *
         * @param {number} userId
         * @return {promise<groupInviteList[]>}  array of groupInfoObjects, invited groups
         */
        groupInviteListByUserId: function(userId) {
            logger.log('getGroups: in groupInviteListByUserId(): ' + userId);
            return new Promise(function(resolve, reject) {
                            
                logger.log('getGroups: in groupInviteListByUserId(): in new Promise');
                var endPoint = urls.endPoints.groupInvitedList;
                endPoint = endPoint.replace('{userId}', userId);
                endPoint = endPoint.replace('{pagesize}', '100');

                var config = {
                    atype: 'GET',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: true,
                    requser: false
                };

                var token = user.publicObjs.accessToken();
                if (token.length > 0) {
                    dataManager.addHeader(config, 'X-AuthToken', token);
                }

                dataManager.addHeader(config, 'X-Api-Version', '2');
                dataManager.addHeader(config, 'X-Application-Key', 'Origin');

                logger.log('getGroups: in groupInviteListByUserId(): dataManager.enQueue:' + endPoint);
                
                var promise = dataManager.enQueue(endPoint, config, 0);
                promise.then(function(response) {
                    logger.log('getGroups: in groupInviteListByUserId(): response:' + response);
                    resolve(response);
                }, function(error) {
                    logger.log('getGroups: in groupInviteListByUserId(): error:' + error);
                    reject(error);
                }).catch(function(error) {
                    logger.log('getGroups: in groupInviteListByUserId(): error:' + error);
                });
            });
        },	

        /**
         * This will return a promise for the room list for the groupGuid specified
         *
         * @param {number} groupGuid
         * @return {promise<roomList[]>}  array of roomInfoObjects
         */
        roomListByGroup: function(groupGuid) {
            return new Promise(function(resolve, reject) {
                            
                var endPoint = urls.endPoints.roomList;
                endPoint = endPoint.replace('{groupGuid}', groupGuid);

                var config = {
                    atype: 'GET',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: true,
                    requser: false
                };

                var token = user.publicObjs.accessToken();
                if (token.length > 0) {
                    dataManager.addHeader(config, 'X-AuthToken', token);
                }

                dataManager.addHeader(config, 'X-Api-Version', '2');
                dataManager.addHeader(config, 'X-Application-Key', 'Origin');

                var promise = dataManager.enQueue(endPoint, config, 0);
                promise.then(function(response) {
                    resolve(response);
                }, function(error) {
                    reject(error);
                }).catch(function(error) {
                });
            });
        },

        /**
         * This will return a promise for the group members list for the groupGuid specified
         *
         * @param {number} groupGuid
         * @return {promise<userList[]>}  array of groupMemberObjects
         */
        memberListByGroup: function(groupGuid, pageStart, pageSize) {
            return new Promise(function(resolve, reject) {
                            
                var endPoint = urls.endPoints.membersList;
                endPoint = endPoint.replace('{groupGuid}', groupGuid);
                endPoint = endPoint.replace('{pagesize}', pageSize);
                endPoint = endPoint.replace('{pagestart}', pageStart);

                var config = {
                    atype: 'GET',
                    headers: [],
                    parameters: [],
                    appendparams: [],
                    reqauth: true,
                    requser: false
                };

                var token = user.publicObjs.accessToken();
                if (token.length > 0) {
                    dataManager.addHeader(config, 'X-AuthToken', token);
                }

                dataManager.addHeader(config, 'X-Api-Version', '2');
                dataManager.addHeader(config, 'X-Application-Key', 'Origin');

                var promise = dataManager.enQueue(endPoint, config, 0);
                promise.then(function(response) {
                    resolve(response);
                }, function(error) {
                    reject(error);
                }).catch(function(error) {
                    logger.log(error, 'groups - memberListByGroup');
                });
            });
        }
			 
	};
	
});