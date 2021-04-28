import os
import os.path
import re
import sys
import fileinput
import string
import datetime

def DoCommand0(command):
    print(command)
    os.system(command)
   
def CreateDirIfNotExist(path):
    if not(os.path.exists(path)):
        os.makedirs(path)
        
def ExtractTimeFromFileName(filename):    
    sTime = "0"
    year = str(datetime.datetime.now().year) 
    for splitName in filename.split("_"):        
        if (splitName.find(year)!= -1 or splitName.find(".gz")!= -1):            
            sTime += splitName            
    sTime = sTime.replace(".log.gz" ,"")
    sTime = sTime.replace("-" ,"")   
    return int(sTime)

def FindLastFetchLogTime(srcLogDir):
    lastFetchTimeStamp = 201201010000000
    
    #Read timeStamp file if one is avaliable
    timeStampFilename = srcLogDir + "/timeStamp.txt"
    if(os.path.exists(timeStampFilename)):
        timeStampFP = open (timeStampFilename, 'r')
        lastFetchTimeStamp = int(timeStampFP.readline()) 
        timeStampFP.close()
    else:
        timeStampFP = open (timeStampFilename, 'w')
        timeStampFP.write(str(lastFetchTimeStamp))
        timeStampFP.close()
    
    return lastFetchTimeStamp
    
def LogFilenamesToInterpret(srcLogDir, lastFetchTimeStamp):
    newLogFiles = []    
    allLogFiles = os.listdir(srcLogDir)
    for file in allLogFiles:
        if(file.find("Slave")!= -1 and file.find("gamereporting")!= -1):            
            time = ExtractTimeFromFileName(file)
            #print time, " ", lastFetchTimeStamp            
            if(time > lastFetchTimeStamp):
                #print "Filter-in " + file                
                newLogFiles.append(file)    
        
    return newLogFiles
    
def doValueCheck(inputStr, isLastNode, delimiter="\t"):
    output =""
    if inputStr != None:
        output += inputStr    
        
    if(isLastNode == 0):   
        output += delimiter
    return output    

GameTypesString = []
GameTypesString.append("FIFA_H2H_Season") 
GameTypesString.append("FIFA_H2H_Cup")
GameTypesString.append("FIFA_OTP_ProRank")
GameTypesString.append("FIFA_OTP_Clubs")
GameTypesString.append("FIFA_Friendlies")
GameTypesString.append("FIFA_Coop")
GameTypesString.append("FIFA_Coop_CUP")
GameTypesString.append("FUT_H2H")
GameTypesString.append("FUT_Online_Tour")
GameTypesString.append("FUT_Offline_PAF")
GameTypesString.append("FUT_Offline_Tour")
GameTypesString.append("FUT_H2H_Season")
GameTypesString.append("FUT_Offline_Season")
GameTypesString.append("FUT_Friendlies")
GameTypesString.append("FUT_Offline_TOTW")
GameTypesString.append("FIFA_Offline_PlayNow")
GameTypesString.append("FIFA_Offline_CreateTour")
GameTypesString.append("FIFA_Offline_Career")
GameTypesString.append("FIFA_MatchDay")
GameTypesString.append("FIFA_LiveFixture")
GameTypesString.append("FIFA_GOTW")
GameTypesString.append("FIFA_Be_A_Pro")
GameTypesString.append("FIFA_Be_A_GoalKeeper")
GameTypesString.append("FIFA_Clubs_CUP")
GameTypesString.append("FUT_Online_HouseRules")
GameTypesString.append("FUT_Offline_HouseRules")
GameTypesString.append("FUT_Online_Friendly_HouseRules")

GameTypesString.append("FUT_ONLINE_FRIENDLY_COMPETITIVE")
GameTypesString.append("FUT_ONLINE_RIVALS")
GameTypesString.append("FUT_OFFLINE_SQUAD_BATTLE")
GameTypesString.append("FUT_ONLINE_CHAMPIONS")

GameTypesString.append("SSF_SEASONS")
GameTypesString.append("SSF_WORLD_TOUR")
GameTypesString.append("SSF_TWIST")
GameTypesString.append("SSF_SINGLE_MATCH")
GameTypesString.append("ORGANIZED_TOURNAMENT_MATCH")
GameTypesString.append("SSF_TWIST_SINGLE_MATCH")
GameTypesString.append("FUT_OFFLINE_DRAFT")
GameTypesString.append("FUT_Online_Squad_Battle")
GameTypesString.append("FUT_ONLINE_ORGANIZED_TOURNAMENT")
 
# Game completed normally in regulation time 
# Game completed normally in overtime time 
# Game was terminated by opponent quits, no stats counted. DNF/loss counted. 
# Game was terminated by consensual quit, no stats/DNF/loss are counted 
# Game was terminated when the user Conceded Defeat(Opponent agreed). Stats/loss are counted, no DNF given 
# Game was terminated when the user Granted Mercy(Opponent agreed). Stats/loss are counted, no DNF given 
# Game was terminated when opponent quit and user wants to count game, stats/DNF/loss are counted 
# Game was terminated by opponent disconnects, no stats counted. DNF/loss counted. 
# Game was terminated when opponent disconnects, user wants no stats/DNF/loss are counted 
# Game was terminated when the user Conceded Defeat(Opponent agreed). Stats/loss are counted, no DNF given 
# Game was terminated when the user Granted Mercy(Opponent agreed). Stats/loss are counted, no DNF given 
# Game was terminated when opponent disconnects, user wants to give Loss/DNF to opponent. All stats count 
# Game was terminated because of a desync 
# Game was terminated because of other reason 
 
OSDKEnhancedGameReportPersistent_StatGameResult = []
OSDKEnhancedGameReportPersistent_StatGameResult.append("COMPLETE_REGULATION")
OSDKEnhancedGameReportPersistent_StatGameResult.append("COMPLETE_OVERTIME")
OSDKEnhancedGameReportPersistent_StatGameResult.append("DNF_QUIT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("FRIENDLY_QUIT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("CONCEDE_QUIT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("MERCY_QUIT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("QUITANDCOUNT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("DNF_DISCONNECT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("FRIENDLY_DISCONNECT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("CONCEDE_DISCONNECT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("MERCY_DISCONNECT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("DISCONNECTANDCOUNT")
OSDKEnhancedGameReportPersistent_StatGameResult.append("DESYNCED")
OSDKEnhancedGameReportPersistent_StatGameResult.append("OTHER")

GameEndReasons = []
GameEndReasons.append("Game_Completed")
GameEndReasons.append("User_Quit")
GameEndReasons.append("Game_Conn_Disconnect")
GameEndReasons.append("Game_Desync")
GameEndReasons.append("Other")
      

def SideResolver(isHome):
    result = "Home"   
    # figure out the disconnect case
    if(isHome == 0):
        result = "Away"
    return result
    
def UserResultResolver(userResult, finishReason):
    result = ""   
    # figure out the disconnect case
    if(userResult == 11 and finishReason != 1):
        result = GameEndReasons[2]
    elif (userResult == 6 or finishReason == 1):
        result = GameEndReasons[1]
    else:
        result = OSDKEnhancedGameReportPersistent_StatGameResult[userResult]
    return result

def IsPreservePrivateAttribute(gameType):
    result = 0  
    if(gameType == "gameType70"):
        result = 1
    elif(gameType == "gameType71"):
        result = 1
    elif(gameType == "gameType72"):
        result = 1
    elif(gameType == "gameType74"):
        result = 1
    elif(gameType == "gameType75"):
        result = 1
    elif(gameType == "gameType76"):
        result = 1
    elif(gameType == "gameType78"):
        result = 1
    elif(gameType == "gameType79"):
        result = 1
    elif(gameType == "gameType83"):
        result = 1
    elif(gameType == "gameType84"):
        result = 1
    elif(gameType == "gameType85"):
        result = 1
    elif(gameType == "gameType87"):
        result = 1
    elif(gameType == "gameType89"):
        result = 1
    elif(gameType == "gameType27"):
        result = 1
    elif(gameType == "gameType28"):
        result = 1
    elif(gameType == "gameType29"):
        result = 1
    elif(gameType == "gameType30"):
        result = 1
    elif(gameType == "gameType32"):
        result = 1
    return result
    
def GameTypeResolver(gameType):
    
    if(gameType == "gameType86"):
        result = GameTypesString[11]
    elif(gameType == "gameType95"):
        result = GameTypesString[17]
    elif(gameType == "gameType0"):
        result = GameTypesString[0]
    elif(gameType == "gameType100"):
        result = GameTypesString[18]
    elif(gameType == "gameType9"):
        result = GameTypesString[3]
    elif(gameType == "gameType88"):
        result = GameTypesString[13]
    elif(gameType == "gameType20"):
        result = GameTypesString[4]
    elif(gameType == "gameType87"):
        result = GameTypesString[12]
    elif(gameType == "gameType82"):
        result = GameTypesString[8]
    elif(gameType == "gameType25"):
        result = GameTypesString[5]
    elif(gameType == "gameType85"):
        result = GameTypesString[10]
    elif(gameType == "gameType5"):
        result = GameTypesString[2]
    elif(gameType == "gameType92"):
        result = GameTypesString[16]
    elif(gameType == "gameType90"):
        result = GameTypesString[15]
    elif(gameType == "gameType81"):
        result = GameTypesString[7]
    elif(gameType == "gameType89"):
        result = GameTypesString[14]
    elif(gameType == "gameType101"):
        result = GameTypesString[19]
    elif(gameType == "gameType102"):
        result = GameTypesString[20]
    elif(gameType == "gameType83"):
        result = GameTypesString[9]
    elif(gameType == "gameType26"):
        result = GameTypesString[6]
    elif(gameType == "gameType1"):
        result = GameTypesString[1]
    elif(gameType == "gameType91"):
        result = GameTypesString[21]
    elif(gameType == "gameType96"):
        result = GameTypesString[22]
    elif(gameType == "gameType13"):
        result = GameTypesString[23]
    elif(gameType == "gameType73"):
        result = GameTypesString[24]
    elif(gameType == "gameType74"):
        result = GameTypesString[25]
    elif(gameType == "gameType72"):
        result = GameTypesString[26]
    elif(gameType == "gameType75"):
        result = GameTypesString[27]
    elif(gameType == "gameType76"):
        result = GameTypesString[28]    
    elif(gameType == "gameType78"):
        result = GameTypesString[29]
    elif(gameType == "gameType79"):
        result = GameTypesString[30]
        
    elif(gameType == "gameType27"):
        result = GameTypesString[31]
    elif(gameType == "gameType28"):
        result = GameTypesString[32]
    elif(gameType == "gameType29"):
        result = GameTypesString[33]
    elif(gameType == "gameType30"):
        result = GameTypesString[34]
    elif(gameType == "gameType31"):
        result = GameTypesString[35]
    elif(gameType == "gameType32"):
        result = GameTypesString[36]
    elif(gameType == "gameType84"):
        result = GameTypesString[37]
    elif(gameType == "gameType71"):
        result = GameTypesString[38]
    elif(gameType == "gameType70"):
        result = GameTypesString[39]
        
    else:
        result = gameType
    return result