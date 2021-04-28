import sys
import fileinput
import string
import json
from json import JSONEncoder
from GameReportHelpers import *

def getFIFA16GameReportAttributesModuleName():
    return __name__
    
def ResolveGameReportAttributesParser(gameType):
    if(gameType == "gameType86"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType95"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType0"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType100"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType9"):
        result = "ClubReportAttribute"  #club
    elif(gameType == "gameType88"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType80"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType20"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType87"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType82"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType25"):
        result = "CoopReportAttribute"   #coop
    elif(gameType == "gameType85"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType5"):
        result = "ClubReportAttribute"   #club
    elif(gameType == "gameType13"):
        result = "ClubReportAttribute"   #club
    elif(gameType == "gameType92"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType90"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType81"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType89"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType101"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType102"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType83"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType84"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType26"):
        result = "CoopReportAttribute"   #coop
    elif(gameType == "gameType1"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType91"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType96"):
        result = "SoloReportAttribute"    
    elif(gameType == "gameType21"):
        result ="SkillGameReportAttribute"
    else:
        result = "BaseMatchAttribute"   
    return result


    
class TitleSpecificParser:
    skuId = ""
    titleId = "19"
    outputMode = "EADP"    
    
    def __init__(self, platform, outputMode = "EADP"): 
        #default to ps4 sku
        self.skuId = "153"
        self.outputMode = outputMode;
        
        if(platform == "xone"):
            self.skuId = "154"
        elif(platform == "pc"):
            self.skuId = "152"
        elif(platform == "ps3"):
            self.skuId = "151"
        elif(platform == "xbl2"):
            self.skuId = "150"
        elif(platform == "ps4-demo"):
            self.skuId = "100000"    
        elif(platform == "xone-demo"):
            self.skuId = "100001"
        elif(platform == "pc-demo"):
            self.skuId = "100002"
        elif(platform == "ps3-demo"):
            self.skuId = "100003"
        elif(platform == "xbl2-demo"):
            self.skuId = "100004"         
             
    
    def ParseAndWriteToFile(self, root, FP):
        #online reports parsing	
        self.numReportNode = 0        
        for eventNode in root.findall('event'):
            if(self.outputMode == "CSV"):
                FP.write(self.ParseReport(eventNode))
            else:
                FP.write(self.ParseReport(eventNode))
            self.numReportNode = self.numReportNode+1
            
    def ParseReport(self, eventNode):
        reportNode = None
        reportString = ""
        prcoessStatusNode = eventNode.find('gamereportsucceeded')
        if (prcoessStatusNode == None):
            prcoessStatusNode = eventNode.find('gamereportdiscrepancy')
            # if (prcoessStatusNode == None):
                # print "Game Report Process failed/orphanned/unknown"
            # else:
                # #reportNode = eventNode.find('gamereportdiscrepancy/gamereports/')
                # #reportStatus="gamereportdiscrepancy"
                # print "Game Report discrepency reports"                    
        else:
            reportNode = eventNode.find('gamereportsucceeded/report')
            reportStatus="gamereportsucceeded"
            
        if(reportNode != None):
            eventTime = eventNode.get('date')                
            reportString = TitleSpecificParser.ParseReportNode(self, reportNode, reportStatus, eventTime)
        return reportString
        
    def ParseReportNode(self, reportNode, reportStatus, eventTime):
        #parse game type, game time and game report id
        skillGames = reportNode.find('report/skillgame')
        #if there is no gameType, it must be skill games report, we skip parsing
        moduleName =  getFIFA16GameReportAttributesModuleName()
        moduleObject = sys.modules[moduleName]
        
        reportString=""
            
        if skillGames == None:           
            gameType = reportNode.find('report/gamereport/gametype').text
            sResolvedGameType = GameTypeResolver(gameType)
            gameTime = str(int(reportNode.find('report/gamereport/gametime').text) / 60)
            gameReportId = reportNode.find('report/gamereport/gamereportid').text        
            commonGameReport = CommonGameReport()
            if(gameType=="gameType25" or gameType=="gameType26"):
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/customcoopgamereport/commongamereport')
                if commongamereportnode != None:
                    CommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)            
            elif(gameType=="gameType9" or gameType=="gameType13"):
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/customclubgamereport/commongamereport')
                if commongamereportnode != None:
                    CommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)            
            else:
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/commongamereport')
                if commongamereportnode != None:
                    CommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)

            playerReports = reportNode.find('report/playerreports')        
            if playerReports != None:
              for entry in playerReports.findall('entry'):
                personaId = entry.get('key')
                if string.atoi(personaId) != 0:                                
                    attrib = None                
                    # Resolve the GameReportAttributesPaser class and                 
                    attributeParser = getattr(moduleObject,ResolveGameReportAttributesParser(gameType))                
                    attrib = attributeParser(reportStatus, eventTime, self.skuId, self.titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
                    attrib.PopulateAttributesFromReport(entry)
                    reportString = reportString + attrib.DumpAttributesToFile(self.outputMode)
        else:                        
            # instantiate the skill game report attribute object                
            attributeParser = getattr(moduleObject,"SkillGameReportAttribute")                
            attrib = attributeParser(reportStatus, eventTime, self.skuId, self.titleId)
            attrib.PopulateAttributesFromReport(reportNode)
            reportString = attrib.DumpAttributesToFile(self.outputMode)    
            
        return reportString
        
class AttributeEncoder(json.JSONEncoder):
    def default(self, obj):
        return obj.__dict__
    
class StartingEleven:       
    def __init__(self):
        self.playerIds = []
        
    def PopulateAttributesFromReport(self, xmlContent, starting11Tag):
        listOfPlayers = xmlContent.findall(starting11Tag)
        for player in listOfPlayers:
            self.playerIds.append(player.text)            
        
class CommonGameReport:    
    ballid = None
    stadiumid = None
    homekitid = None
    homestartingxi = None
    awaykitid = None  
    awaystartingxi = None
    
    def __init__(self):
        self.homestartingxi = StartingEleven()
        self.awaystartingxi = StartingEleven()
        self.ballid = ""
        self.stadiumid = ""
        self.homekitid = ""        
        self.awaykitid = ""        
    
    #expect xmlContent is of commongamereport node
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('ballid')
        if(node != None):
            self.ballid = node.text
        
        node = xmlContent.find('stadiumid')
        if(node != None):
            self.stadiumid = node.text
        
        node = xmlContent.find('homekitid')
        if(node != None):
            self.homekitid = node.text        
        
        node = xmlContent.find('awaykitid')
        if(node != None):
            self.awaykitid = node.text
            
        node = xmlContent.find('homestartingxi')
        if(node != None):
            StartingEleven.PopulateAttributesFromReport(self.homestartingxi, node, "homestartingxi")
            
        node = xmlContent.find('awaystartingxi')
        if(node != None):
            StartingEleven.PopulateAttributesFromReport(self.awaystartingxi, node, "awaystartingxi")      
        
              
# BaseMatchAttribute includes attributes from OSDKGamereport AND OSDKPlayerReport       
class BaseMatchAttribute:
    reportStatus = 0
    eventTime = 0
    skuId = 0
    titleId = 0
    gameTime = ""
    gameReportId = 0
    gameType = ""
    
    finishReason = 0
    gameResult = 0
    nucleusId = 0
    personaId = 0
    persona = ""
    userResult = ""
    sUserResult = ""
    dnfWin = 0
    commonGameReport = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        self.reportStatus = reportStatus
        self.eventTime = eventTime
        self.skuId = skuId
        self.titleId = titleId
        self.gameTime = gameTime
        self.gameReportId = gameReportId
        self.gameType = gameType
        self.personaId = personaId
        self.commonGameReport = commonGameReport
        self.finishReason = ""
        self.gameResult = ""
        self.nucleusId = ""
        self.persona = ""
        self.userResult = ""
        self.sUserResult = ""
        self.dnfWin = ""
    
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('osdkplayerreport/finishreason')
        if(node != None):
            self.finishReason = node.text
            
        node = xmlContent.find('osdkplayerreport/gameresult')
        if(node != None):    
            self.gameResult = node.text
            
        node = xmlContent.find('osdkplayerreport/nucleusid')
        if(node != None):    
            self.nucleusId = node.text
        
        node = xmlContent.find('osdkplayerreport/userresult')
        if(node != None):                
            self.userResult = node.text          
        
        if(self.userResult != "" and self.finishReason != ""):
            self.sUserResult = UserResultResolver(int(self.userResult), int(self.finishReason))        

        node = xmlContent.find('osdkplayerreport/winnerbydnf')
        if(node != None):
            self.dnfWin = node.text
    
    def DumpAttributesToFile(self, outputMode="EADP"):
        outputString = ""
        if(outputMode == "CSV"):
            delimiter=","
            outputString += doValueCheck(self.eventTime, 0, delimiter)
            outputString += doValueCheck(self.gameReportId, 0, delimiter)
            outputString += doValueCheck(self.nucleusId, 0, delimiter)                        
            outputString += doValueCheck(self.personaId, 0, delimiter)
            outputString += doValueCheck(self.skuId, 0, delimiter)
            outputString += doValueCheck(self.titleId, 0, delimiter)
            outputString += doValueCheck(GameTypeResolver(self.gameType), 0, delimiter)
            outputString += doValueCheck(self.gameTime, 0, delimiter)
            outputString += doValueCheck(self.sUserResult, 0, "\n")            
        else:
            outputString += doValueCheck(self.eventTime, 0)
            outputString += doValueCheck(self.gameReportId, 0)
            outputString += doValueCheck(self.nucleusId, 0)                        
            outputString += doValueCheck(self.personaId, 0)  
            outputString += doValueCheck(self.skuId, 0)
            outputString += doValueCheck(self.titleId, 0)
            
            attribJson = AttributeEncoder().encode(self)
            outputString += doValueCheck(attribJson, 0, "\n")  
            
        return outputString        
    
    
class CommonPlayerReport():
    assists = 0
    goalsconceded = 0
    corners = 0
    passattempts = 0
    passesmade = 0
    rating = 0
    saves = 0
    shots = 0
    tackleattempts = 0
    tacklesmade = 0
    fouls = 0
    goals = 0
    interceptions = 0
    offsides = 0
    owngoals = 0
    pkgoals = 0
    possession = 0
    redcard = 0
    shotsongoal = 0
    unadjustedscore = 0
    yellowcard = 0
    side = 0
    
    def __init__(self):
        self.assists = ""
        self.goalsconceded = ""
        self.corners = ""
        self.passattempts = ""
        self.passesmade = ""
        self.rating = ""
        self.saves = ""
        self.shots = ""
        self.tackleattempts = ""
        self.tacklesmade = ""
        self.fouls = ""
        self.goals = ""
        self.interceptions = ""
        self.offsides = ""
        self.owngoals = ""
        self.pkgoals = ""
        self.possession = ""
        self.redcard = ""
        self.shotsongoal = ""
        self.unadjustedscore = ""
        self.yellowcard = ""
        self.side = ""
        
    def PopulateAttributesFromReport(self, xmlContent):
    
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/assists')
        if(node != None):
            self.assists = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/goalsconceded')
        if(node != None):            
            self.goalsconceded= node.text

        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/corners')
        if(node != None):             
            self.corners = node.text

        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/passattempts')
        if(node != None):                
            self.passattempts = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/passesmade')
        if(node != None): 
            self.passesmade = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/rating')
        if(node != None): 
            self.rating = node.text

        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/saves')
        if(node != None):            
            self.saves = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/shots')
        if(node != None):              
            self.shots = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/tackleattempts')
        if(node != None):   
            self.tackleattempts = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/tacklesmade')
        if(node != None):               
            self.tacklesmade = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/fouls')
        if(node != None):    
            self.fouls = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/goals')
        if(node != None):  
            self.goals = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/interceptions')
        if(node != None):  
            self.interceptions = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/offsides')
        if(node != None): 
            self.offsides = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/owngoals')
        if(node != None): 
            self.owngoals = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/pkgoals')
        if(node != None): 
            self.pkgoals = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/possession')
        if(node != None): 
            self.possession = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/redcard')
        if(node != None): 
            self.redcard = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/shotsongoal')
        if(node != None):             
            self.shotsongoal = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/unadjustedscore')
        if(node != None):  
            self.unadjustedscore = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/yellowcard')
        if(node != None):              
            self.yellowcard = node.text
            
        node = xmlContent.find('osdkplayerreport/home')
        if(node != None):    
            self.side = SideResolver(int(node.text))

class H2HCustomPlayerDataGameReport():
    controls = 0
    goalagainst = 0   
    nbguests = 0   
    shotsagainst = 0   
    shotsagainsto = 0    
    team = 0  
    teamrating = 0   
    teamname = 0   
    wenttopk = 0            
      
    def __init__(self):
        self.controls = ""
        self.goalagainst = ""   
        self.nbguests = ""   
        self.shotsagainst = ""   
        self.shotsagainsto = ""    
        self.team = ""  
        self.teamrating = ""   
        self.teamname = ""   
        self.wenttopk = ""   
    
    def PopulateAttributesFromReport(self, xmlContent):

        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/controls')
        if(node != None):     
            self.controls = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/goalagainst')
        if(node != None):     
            self.goalagainst = node.text  
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/nbguests')
        if(node != None):   
            self.nbguests = node.text   
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/shotsagainst')
        if(node != None): 
            self.shotsagainst = node.text   
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/shotsagainstongoal')
        if(node != None): 
            self.shotsagainstongoal = node.text    
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/team')
        if(node != None): 
            self.team = node.text  
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/teamrating')
        if(node != None): 
            self.teamrating = node.text   
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/teamname')
        if(node != None): 
            self.teamname = node.text   
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcustomplayerdata/wenttopk')
        if(node != None): 
            self.wenttopk = node.text               
        
class SeasonalPlayReport():
    currentdivision = 0
    woncompetition = 0
    wonleaguetitle = 0
    updatedivision = 0
    seasonid = 0
    cup_id = 0
    gamenumber = 0
   
    def __init__(self):
        self.currentdivision = ""
        self.woncompetition = ""
        self.wonleaguetitle = ""
        self.updatedivision = ""
        self.seasonid = ""
        self.cup_id = ""
        self.gamenumber = ""   
        
    def PopulateAttributesFromReport(self, xmlContent):             
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/currentdivision')
        if(node != None):
            self.currentdivision = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/woncompetition')
        if(node != None):
            self.woncompetition = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/wonleaguetitle')
        if(node != None):
            self.wonleaguetitle = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/updatedivision')
        if(node != None):
            self.updatedivision = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/seasonid')
        if(node != None):            
            self.seasonid = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/cup_id')
        if(node != None):            
            self.cup_id = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/seasonalplaydata/gamenumber')
        if(node != None): 
            self.gamenumber = node.text

class SoloCustomPlayerData:
    goalagainst = 0
    losses = 0
    result = 0
    shotsagainst = 0
    shotsagainstongoal = 0
    team = 0
    ties = 0
    wins = 0
    
    def __init__(self):
        self.goalagainst = ""
        self.losses = ""
        self.result = ""
        self.shotsagainst = ""
        self.shotsagainstongoal = ""
        self.team = ""
        self.ties = ""
        self.wins = ""
    
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/goalagainst')
        if(node != None): 
            self.goalagainst = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/losses')
        if(node != None): 
            self.losses = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/result')
        if(node != None): 
            self.result = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/shotsagainst')
        if(node != None): 
            self.shotsagainst = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/shotsagainstongoal')
        if(node != None): 
            self.shotsagainstongoal = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/team')
        if(node != None):        
            self.team = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/ties')
        if(node != None):   
            self.ties = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/solocustomplayerdata/wins')
        if(node != None):  
            self.wins = node.text
        
#-------------------------------------------------------------------------------
class H2HReportAttribute(BaseMatchAttribute):
    commonPlayer = 0
    customPlayer = 0
    seasonalPlay = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.commonPlayer = CommonPlayerReport()
        self.customPlayer = H2HCustomPlayerDataGameReport()
        self.seasonalPlay = SeasonalPlayReport()
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CommonPlayerReport.PopulateAttributesFromReport(self.commonPlayer, xmlContent)
        H2HCustomPlayerDataGameReport.PopulateAttributesFromReport(self.customPlayer,xmlContent)
        SeasonalPlayReport.PopulateAttributesFromReport(self.seasonalPlay,xmlContent)    
    
class SoloReportAttribute(BaseMatchAttribute):
    commonPlayer = 0
    customPlayer = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.commonPlayer = CommonPlayerReport()
        self.customPlayer = SoloCustomPlayerData()
        
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CommonPlayerReport.PopulateAttributesFromReport(self.commonPlayer, xmlContent)
        SoloCustomPlayerData.PopulateAttributesFromReport(self.customPlayer,xmlContent)

class CoopReportAttribute(BaseMatchAttribute):
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)


class ClubReportAttribute(BaseMatchAttribute):
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        
class SkillGameReportAttribute:
    reportStatus = 0
    eventTime = 0
    skuId = 0
    titleId = 0
    gameReportId=0
    gameType=0
    countrycode = 0
    nucleusId = 0
    personaId=0
    skillgame=0
    score=0   
    
    def __init__(self, reportStatus, eventTime, skuId, titleId):
        self.reportStatus = reportStatus
        self.eventTime = eventTime
        self.skuId = skuId
        self.titleId = titleId
        self.gameReportId = ""
        self.gameType = ""
        self.countrycode = ""
        self.nucleusId = ""
        self.personaId = ""
        self.skillgame = ""
        self.score = ""           
    
    def PopulateAttributesFromReport(self, xmlContent):        
        node = xmlContent.find('report/countrycode')
        if(node != None):  
            self.countrycode = node.text    
            
        node = xmlContent.find('report/playerid')
        if(node != None):  
            self.personaId = node.text  
            
        #Please be aware below, currently skill game only has personaId, once nucleusId is supplied please hook that attribute up properly
        self.nucleusId = self.personaId
        
        node = xmlContent.find('report/skillgame')
        if(node != None):  
            self.skillgame = node.text    
            
        node = xmlContent.find('report/score')
        if(node != None): 
            self.score = node.text
            
        node = xmlContent.find('gamereportingid')
        if(node != None): 
            self.gameReportId = node.text   
            
        node = xmlContent.find('gamereportname')
        if(node != None): 
            self.gameType = node.text            
        
    def DumpAttributesToFile(self, outputMode="EADP"):
        outputString = ""
        if(outputMode == "CSV"):
            delimiter=","
            outputString += doValueCheck(self.eventTime, 0, delimiter)
            outputString += doValueCheck(self.gameReportId, 0, delimiter)
            outputString += doValueCheck(self.nucleusId, 0, delimiter)                        
            outputString += doValueCheck(self.personaId, 0, delimiter)
            outputString += doValueCheck(self.skuId, 0, delimiter)
            outputString += doValueCheck(self.titleId, 0, delimiter)
            outputString += doValueCheck("SkillGame", 0, delimiter)
            outputString += doValueCheck("1000", 0, delimiter)
            outputString += doValueCheck("Completed", 0, "\n")            
        else:
            outputString += doValueCheck(self.eventTime, 0)
            outputString += doValueCheck(self.gameReportId, 0)
            outputString += doValueCheck(self.nucleusId, 0)                        
            outputString += doValueCheck(self.personaId, 0)
            outputString += doValueCheck(self.skuId, 0)
            outputString += doValueCheck(self.titleId, 0)        
            attribJson = AttributeEncoder().encode(self)
            outputString += doValueCheck(attribJson, 0, "\n")  
            
        return outputString

