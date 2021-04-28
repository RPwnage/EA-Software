import sys
import fileinput
import string
import json
from json import JSONEncoder
from GameReportHelpers import *

def getFIFA20GameReportAttributesModuleName():
    return __name__
    
def ResolveGameReportAttributesParser(gameType):
    if(gameType == "gameType86"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType95"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType33"):  #SSF Season
        result = "SSFSeasonReportAttribute"
    elif(gameType == "gameType28"):  #SSF World Tour
        result = "SSFSoloReportAttribute"
    elif(gameType == "gameType29"):  #SSF Twist
        result = "SSFSoloReportAttribute"
    elif(gameType == "gameType30"):  #SSF Single Match
        result = "SSFSoloReportAttribute"
    elif(gameType == "gameType31"):  #1st party Tournament
        result = "H2HReportAttribute"
    elif(gameType == "gameType32"):  #SSF Twist Single Match
        result = "SSFSoloReportAttribute"
    elif(gameType == "gameType0"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType100"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType120"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType201"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType79"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType78"):
        result = "FUTSoloReportAttribute"
    elif(gameType == "gameType76"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType75"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType70"):  #FUT 1st party tournament
        result = "H2HReportAttribute"
    elif(gameType == "gameType71"):
        result = "FUTOTPReportAttribute"
    elif(gameType == "gameType72"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType73"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType74"):
        result = "FUTSoloReportAttribute"
    elif(gameType == "gameType9"):
        result = "ClubReportAttribute"  #club
    elif(gameType == "gameType88"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType80"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType20"):
        result = "H2HReportAttribute"
    elif(gameType == "gameType87"):
        result = "FUTSoloReportAttribute"
    elif(gameType == "gameType82"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType25"):
        result = "CoopReportAttribute"   #coop
    elif(gameType == "gameType85"):
        result = "FUTSoloReportAttribute"
    elif(gameType == "gameType5"):
        result = "ClubReportAttribute"   #club
    elif(gameType == "gameType13"):
        result = "ClubReportAttribute"   #club
    elif(gameType == "gameType92"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType90"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType81"):
        result = "FUTH2HReportAttribute"
    elif(gameType == "gameType89"):
        result = "FUTSoloReportAttribute"
    elif(gameType == "gameType101"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType102"):
        result = "SoloReportAttribute"
    elif(gameType == "gameType83"):
        result = "FUTSoloReportAttribute"
    elif(gameType == "gameType84"):
        result = "FUTSoloReportAttribute"
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
    titleId = "24"
    outputMode = "EADP"    
    
    def __init__(self, platform, outputMode = "EADP"): 
        #default to ps4 sku
        self.skuId = "185"
        self.outputMode = outputMode
        
        if(platform == "xone"):
            self.skuId = "187"
        elif(platform == "pc"):
            self.skuId = "184"
        elif(platform == "nx"):
            self.skuId = "186"      
             
    
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
        moduleName =  getFIFA20GameReportAttributesModuleName()
        moduleObject = sys.modules[moduleName]
        
        reportString=""
            
        if skillGames == None:           
            gameType = reportNode.find('report/gamereport/gametype').text
            sResolvedGameType = GameTypeResolver(gameType)
            gameTime = str(int(reportNode.find('report/gamereport/gametime').text) / 60)
            gameReportId = reportNode.find('report/gamereport/gamereportid').text        
			
            matchdifficulty = ""
            node = reportNode.find('report/gamereport/customgamereport/matchdifficulty')
            if(node != None):                
                matchdifficulty = node.text   
			
            
            commonGameReport = CommonGameReport()
            if(gameType=="gameType25" or gameType=="gameType26"):
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/customcoopgamereport/commongamereport')
                if commongamereportnode != None:
                    CommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)            
            elif(gameType=="gameType9" or gameType=="gameType13"):
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/customclubgamereport/commongamereport')
                if commongamereportnode != None:
                    CommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)
            if(gameType=="gameType33" or gameType=="gameType28" or gameType=="gameType29" or gameType=="gameType30" or gameType=="gameType32"):
                commonGameReport = SSFCommonGameReport()
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/customcoopgamereport/ssfseasonsgamereport/commongamereport')
                if commongamereportnode != None:
                   SSFCommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)                     
            else:
                commongamereportnode = reportNode.find('report/gamereport/customgamereport/commongamereport')
                if commongamereportnode != None:
                    CommonGameReport.PopulateAttributesFromReport(commonGameReport, commongamereportnode)

            # FUT Collator Report
            enhancedReportNode = reportNode.find('report/enhancedreport')
            enhanceReport = ""
            if(enhancedReportNode != None):
                enhanceReport = EnhancedCollatorReport()
                EnhancedCollatorReport.PopulateAttributesFromReport(enhanceReport, enhancedReportNode)

            playerReports = reportNode.find('report/playerreports')        
            if playerReports != None:
              for entry in playerReports.findall('entry'):
                personaId = entry.get('key')
                if string.atoi(personaId) != 0:                                
                    attrib = None                
                    # Resolve the GameReportAttributesPaser class and                 
                    parserType = ResolveGameReportAttributesParser(gameType)
                    attributeParser = getattr(moduleObject, parserType)                
                    attrib = attributeParser(reportStatus, eventTime, self.skuId, self.titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
                    
                    attrib.PopulateAttributesFromReport(entry)
                    attrib.matchdifficulty = matchdifficulty
                    if parserType == "FUTH2HReportAttribute":
                        FUTH2HReportAttribute.AppendEnhancedCollatorReport(attrib, enhanceReport, enhancedReportNode)
                    
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
    gameendreason = -64
    nucleusId = 0
    personaId = 0
    persona = ""
    userResult = ""
    sUserResult = ""
    dnfWin = 0
    commonGameReport = 0
    
	
    matchdifficulty = 0
    
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
        self.gameendreason = ""
        self.nucleusId = ""
        self.persona = ""
        self.userResult = ""
        self.sUserResult = ""
        self.dnfWin = ""        
        self.matchdifficulty = ""
    
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('osdkplayerreport/finishreason')
        if(node != None):
            self.finishReason = node.text
            
        node = xmlContent.find('osdkplayerreport/gameresult')
        if(node != None):    
            self.gameResult = node.text

        privateintattributemap = xmlContent.find('osdkplayerreport/privateplayerreport/privateintattributemap')
        if(privateintattributemap != None):
            for entry in privateintattributemap.iter('entry'):
                if(entry.get('key') == 'GAMEENDREASON'):
                    self.gameendreason = entry.text
            
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
        self.commonDetailReport = CommonDetailReport()
        
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

        CommonDetailReport.PopulateAttributesFromReport(self.commonDetailReport, xmlContent)
        
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

class H2HCollationPlayerDataGameReport():
    goals = 0
    ownGoals = 0
    pkGoals = 0
    goalsConceded = 0
    fouls = 0
    yellowCard = 0
    redCard = 0
    offsides = 0
    
    def __init__(self):
        self.goals = ""
        self.ownGoals = ""
        self.pkGoals = ""
        self.goalsConceded = ""
        self.fouls = ""
        self.yellowCard = ""
        self.redCard = ""
        self.offsides = ""
    
    def PopulateAttributesFromReport(self, xmlContent):

        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/goals')
        if(node != None):     
            self.goals = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/owngoals')
        if(node != None):     
            self.ownGoals = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/pkgoals')
        if(node != None):     
            self.pkGoals = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/goalsconceded')
        if(node != None):     
            self.goalsConceded = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/fouls')
        if(node != None):     
            self.fouls = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/yellowcard')
        if(node != None):     
            self.yellowCard = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/redcard')
        if(node != None):     
            self.redCard = node.text
        
        node = xmlContent.find('osdkplayerreport/customplayerreport/h2hcollationplayerdata/offsides')
        if(node != None):     
            self.offsides = node.text
            
class CommonDetailReport():
    penaltiesAwarded = 0
    penaltiesScored = 0
    penaltiesSaved = 0
    penaltiesMissed = 0
    averagePossessionLength = 0
    passesIntercepted = 0
    averageFatigueAfterNinety = 0
    injuriesRed = 0
    
    def __init__(self):
        self.penaltiesAwarded = ""
        self.penaltiesScored = ""
        self.penaltiesSaved = ""
        self.penaltiesMissed = ""
        self.averagePossessionLength = ""
        self.passesIntercepted = ""
        self.averageFatigueAfterNinety = ""
        self.injuriesRed = ""
    
    def PopulateAttributesFromReport(self, xmlContent):
      
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/penaltiesawarded')
        if(node != None):     
            self.penaltiesAwarded = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/penaltiesscored')
        if(node != None):     
            self.penaltiesScored = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/penaltiessaved')
        if(node != None):     
            self.penaltiesSaved = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/penaltiesmissed')
        if(node != None):     
            self.penaltiesMissed = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/averagepossessionlength')
        if(node != None):     
            self.averagePossessionLength = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/passesintercepted')
        if(node != None):     
            self.passesIntercepted = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/averagefatigueafterninety')
        if(node != None):     
            self.averageFatigueAfterNinety = node.text
            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/commondetailreport/injuriesred')
        if(node != None):     
            self.injuriesRed = node.text
            
class CPUStatReport():
    gid = 0
    aiAst = 0
    aiCS = 0
    aiCo = 0
    aiFou = 0
    aiGC = 0
    aiGo = 0
    aiInt = 0
    aiMOTM = 0
    aiOG = 0
    aiOff = 0
    aiPA = 0
    aiPKG = 0
    aiPM = 0
    aiPo = 0
    aiRC = 0
    aiRa = 0
    aiSav = 0
    aiSoG = 0
    aiTA = 0
    aiTM = 0
    aiYC = 0
    futMatchResult = 0
    futMatchFlags = 0
    futMatchTime = 0
    player_division_id = 0
    
    def __init__(self):
        self.gid = ""
        self.aiAst = ""
        self.aiCS = ""
        self.aiCo = ""
        self.aiFou = ""
        self.aiGC = ""
        self.aiGo = ""
        self.aiInt = ""
        self.aiMOTM = ""
        self.aiOG = ""
        self.aiOff = ""
        self.aiPA = ""
        self.aiPKG = ""
        self.aiPM = ""
        self.aiPo = ""
        self.aiRC = ""
        self.aiRa = ""
        self.aiSav = ""
        self.aiSoG = ""
        self.aiTA = ""
        self.aiTM = ""
        self.aiYC = ""        
        self.futMatchResult = ""
        self.futMatchFlags = ""
        self.futMatchTime = ""
        self.player_division_id = ""
    
    def PopulateAttributesFromReport(self, xmlContent):
    
        privateintattributemap = xmlContent.find('osdkplayerreport/privateplayerreport/privateintattributemap')
        if(privateintattributemap != None):
            for entry in privateintattributemap.iter('entry'):
                if(entry.get('key') == 'gid'):
                    self.gid = entry.text
                elif(entry.get('key') == 'aiAst'):
                    self.aiAst = entry.text
                elif(entry.get('key') == 'aiCS'):
                    self.aiCS = entry.text
                elif(entry.get('key') == 'aiCo'):
                    self.aiCo = entry.text
                elif(entry.get('key') == 'aiFou'):
                    self.aiFou = entry.text
                elif(entry.get('key') == 'aiGC'):
                    self.aiGC = entry.text
                elif(entry.get('key') == 'aiInt'):
                    self.aiInt = entry.text
                elif(entry.get('key') == 'aiMOTM'):
                    self.aiMOTM = entry.text
                elif(entry.get('key') == 'aiOG'):
                    self.aiOG = entry.text
                elif(entry.get('key') == 'aiOff'):
                    self.aiOff = entry.text
                elif(entry.get('key') == 'aiPA'):
                    self.aiPA = entry.text
                elif(entry.get('key') == 'aiPKG'):
                    self.aiPKG = entry.text
                elif(entry.get('key') == 'aiPM'):
                    self.aiPM = entry.text
                elif(entry.get('key') == 'aiPo'):
                    self.aiPo = entry.text
                elif(entry.get('key') == 'aiRC'):
                    self.aiRC = entry.text
                elif(entry.get('key') == 'aiRa'):
                    self.aiRa = entry.text
                elif(entry.get('key') == 'aiSav'):
                    self.aiSav = entry.text
                elif(entry.get('key') == 'aiSoG'):
                    self.aiSoG = entry.text
                elif(entry.get('key') == 'aiTA'):
                    self.aiTA = entry.text
                elif(entry.get('key') == 'aiTM'):
                    self.aiTM = entry.text
                elif(entry.get('key') == 'aiYC'):
                    self.aiYC = entry.text
                elif(entry.get('key') == 'futMatchResult'):
                    self.futMatchResult = entry.text
                elif(entry.get('key') == 'futMatchFlags'):
                    self.futMatchFlags = entry.text
                elif(entry.get('key') == 'futMatchTime'):
                    self.futMatchTime = entry.text
                elif(entry.get('key') == 'player_division_id'):
                    self.player_division_id = entry.text
                
            
        
            
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
    CPUStats = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.commonPlayer = CommonPlayerReport()
        self.customPlayer = H2HCustomPlayerDataGameReport()
        self.seasonalPlay = SeasonalPlayReport()
        self.CPUStats = CPUStatReport()        
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CommonPlayerReport.PopulateAttributesFromReport(self.commonPlayer, xmlContent)
        H2HCustomPlayerDataGameReport.PopulateAttributesFromReport(self.customPlayer,xmlContent)
        SeasonalPlayReport.PopulateAttributesFromReport(self.seasonalPlay,xmlContent)
        CPUStatReport.PopulateAttributesFromReport(self.CPUStats, xmlContent)

class CollisionInstance():
    def __init__(self):
        self.member = ""
        self.parent = ""
        self.root = ""
        self.type = ""
        self.value0 = ""
        self.value1 = ""
    
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('member')
        if(node != None):
            self.member = node.text
            
        node = xmlContent.find('parent')
        if(node != None):
            self.parent = node.text
            
        node = xmlContent.find('root')
        if(node != None):
            self.root = node.text
        
        node = xmlContent.find('type')
        if(node != None):
            self.type = node.text
            
        node = xmlContent.find('value0')
        if(node != None):
            self.value0 = node.text
            
        node = xmlContent.find('value1')
        if(node != None):
            self.value1 = node.text

class EnhancedCollatorReport():
    def __init__(self):
        self.reportCount = ""
        self.collision = ""
        self.validReport = ""
        self.invalidPlayerMap = ""
        self.critMissMatch = ""
        self.collisionList = []
    
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('reportcount')
        if(node != None):
            self.reportCount = node.text
            
        node = xmlContent.find('collision')
        if(node != None):
            self.collision = node.text
            
        node = xmlContent.find('validreport')
        if(node != None):
            self.validReport = node.text
            
        node = xmlContent.find('invalidplayermap')
        if(node != None):
            self.invalidPlayerMap = node.text
            
        node = xmlContent.find('critmissmatch')
        if(node != None):
            self.critMissMatch = node.text
        
        collisionListNode = xmlContent.find('collisionlist')
        if(collisionListNode != None):
            self.collisionList = []
            for collisionNode in collisionListNode:
                collision = CollisionInstance()
                CollisionInstance.PopulateAttributesFromReport(collision, collisionNode)
                self.collisionList.append(collision)

class PlayerCollatorReport():
    def _init_(self):
        isBase = ""
        isDnfReport = ""
        matchResult = ""
        reportRcvd = ""
        stdRating = ""
        cmpRating = ""
        
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('isbasereport')
        if(node != None):
            self.isBase = node.text
        
        node = xmlContent.find('isdnfreport')
        if(node != None):
            self.isDnfReport = node.text
            
        node = xmlContent.find('matchresult')
        if(node != None):
            self.matchResult = node.text
            
        node = xmlContent.find('reportreceived')
        if(node != None):
            self.reportRcvd = node.text
            
        node = xmlContent.find('standardrating')
        if(node != None):
            self.stdRating = node.text
            
        node = xmlContent.find('competitiverating')
        if(node != None):
            self.cmpRating = node.text

class FUTH2HReportAttribute(H2HReportAttribute):
    collationPlayerData = ""    
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        H2HReportAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.collationPlayerData = H2HCollationPlayerDataGameReport()        

    def AppendEnhancedCollatorReport(self, report, enhancedReportNode):
        if(enhancedReportNode != None):
            self.enhancedReport = report
            playerReportMapNode = enhancedReportNode.find('playerreportmap')
            if(playerReportMapNode != None):
                for entry in playerReportMapNode:
                    if(entry.get('key') == self.personaId):
                        playerReportNode = entry.find('individualplayerreport')
                        if(playerReportNode != None):
                            self.playerCollatorReport = PlayerCollatorReport()
                            PlayerCollatorReport.PopulateAttributesFromReport(self.playerCollatorReport, playerReportNode)

    def PopulateAttributesFromReport(self, xmlContent):
        H2HReportAttribute.PopulateAttributesFromReport(self, xmlContent)
        H2HCollationPlayerDataGameReport.PopulateAttributesFromReport(self.collationPlayerData, xmlContent)        
    
class SoloReportAttribute(BaseMatchAttribute):
    commonPlayer = 0
    customPlayer = 0
    seasonalPlay = 0
    CPUStats = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.gameendreason = 0;
        self.commonPlayer = CommonPlayerReport()
        self.customPlayer = SoloCustomPlayerData()
        self.seasonalPlay = SeasonalPlayReport()
        self.CPUStats = CPUStatReport()    
        
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CommonPlayerReport.PopulateAttributesFromReport(self.commonPlayer, xmlContent)
        SoloCustomPlayerData.PopulateAttributesFromReport(self.customPlayer,xmlContent)
        SeasonalPlayReport.PopulateAttributesFromReport(self.seasonalPlay,xmlContent)
        CPUStatReport.PopulateAttributesFromReport(self.CPUStats, xmlContent)

class FUTSoloReportAttribute(SoloReportAttribute):
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        SoloReportAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)            
    
    def PopulateAttributesFromReport(self, xmlContent):
        SoloReportAttribute.PopulateAttributesFromReport(self, xmlContent)        
    
class CoopReportAttribute(BaseMatchAttribute):
    CPUStats = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.CPUStats = CPUStatReport()    
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CPUStatReport.PopulateAttributesFromReport(self.CPUStats, xmlContent)


class ClubReportAttribute(BaseMatchAttribute):
    CPUStats = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.CPUStats = CPUStatReport()    
        
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CPUStatReport.PopulateAttributesFromReport(self.CPUStats, xmlContent)

class FUTOTPReportAttribute(BaseMatchAttribute):
    CPUStats = 0
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.CPUStats = CPUStatReport()    
    
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        CPUStatReport.PopulateAttributesFromReport(self.CPUStats, xmlContent)

#-------------------------------- SSF ----------------------------------

class SSFCommonGameReport:   
    wentToPk = None
    isRivalMatch = None
    ballid = None
    stadiumid = None
    homeCrestId = None  
    awayCrestId = None
    
    def __init__(self):
        self.wentToPk = ""        
        self.isRivalMatch = ""
        self.ballid = ""
        self.stadiumid = ""
        self.homeCrestId = ""
        self.awayCrestId = ""        
    
    #expect xmlContent is of commongamereport node
    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('ballId')
        if(node != None):
            self.ballid = node.text
        
        node = xmlContent.find('stadiumId')
        if(node != None):
            self.stadiumid = node.text
        
        node = xmlContent.find('wentToPk')
        if(node != None):
            self.wentToPk = node.text        
        
        node = xmlContent.find('isRivalMatch')
        if(node != None):
            self.isRivalMatch = node.text
            
        node = xmlContent.find('homeCrestId')
        if(node != None):
            self.homeCrestId = node.text
            
        node = xmlContent.find('awayCrestId')
        if(node != None):
            self.awayCrestId = node.text

class SSFCommonStatsReport:
    goals = 0
    ownGoals = 0	
    goalsConceded = 0	
    pkGoals = 0	
    possession = 0	  
    assists = 0	
    shots = 0	
    shotsOnGoal = 0	
    passAttempts = 0	
    passesMade = 0	
    interceptions = 0	
    tackleAttempts = 0	
    tacklesMade = 0	
    blocks = 0	
    rating = 0	
    saves = 0	
    fouls = 0	
    yellowCard = 0	
    redCard = 0	
    corners = 0	
    offsides = 0	
    unadjustedScore = 0	
    hasMOTM = 0	
    hasCleanSheets = 0		
    penaltiesAwarded = 0	
    penaltiesScored = 0	
    penaltiesSaved = 0	
    penaltiesMissed = 0	
    averagePossessionLength = 0	
    passesIntercepted = 0	
    averageFatigueAfterNinety	 = 0	
    injuriesRed = 0	
    
    def __init__(self):
        self.goals = ""
        self.ownGoals = ""	
        self.goalsConceded = ""	
        self.pkGoals = ""	
        self.possession = ""	  
        self.assists = ""	
        self.shots = ""	
        self.shotsOnGoal = ""	
        self.passAttempts = ""	
        self.passesMade = ""	
        self.interceptions = ""	
        self.tackleAttempts = ""	
        self.tacklesMade = ""	
        self.blocks = ""	
        self.rating = ""	
        self.saves = ""	
        self.fouls = ""	
        self.yellowCard = ""	
        self.redCard = ""	
        self.corners = ""	
        self.offsides = ""	
        self.unadjustedScore = ""	
        self.hasMOTM = ""	
        self.hasCleanSheets = ""		
        self.penaltiesAwarded = ""	
        self.penaltiesScored = ""	
        self.penaltiesSaved = ""	
        self.penaltiesMissed = ""	
        self.averagePossessionLength = ""	
        self.passesIntercepted = ""	
        self.averageFatigueAfterNinety	 = ""	
        self.injuriesRed = ""	

    def PopulateAttributesFromReport(self, xmlContent):
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/goals')
        if(node != None):
            self.goals = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/owngoals')
        if(node != None):
            self.ownGoals = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/goalsconceded')
        if(node != None):
            self.goalsConceded = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/pkgoals')
        if(node != None):
            self.pkGoals = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/possession')
        if(node != None):
            self.possession = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/assists')
        if(node != None):
            self.assists = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/shots')
        if(node != None):
            self.shots = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/shotsongoal')
        if(node != None):
            self.shotsOnGoal = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/passattempts')
        if(node != None):
            self.passAttempts = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/passesmade')
        if(node != None):
            self.passesMade = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/interceptions')
        if(node != None):
            self.interceptions = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/tackleattempts')
        if(node != None):
            self.tackleAttempts = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/tacklesmade')
        if(node != None):
            self.tacklesMade = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/blocks')
        if(node != None):
            self.blocks = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/rating')
        if(node != None):
            self.rating = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/saves')
        if(node != None):
            self.saves = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/fouls')
        if(node != None):
            self.fouls = node.text            
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/yellowcard')
        if(node != None):
            self.yellowCard = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/redcard')
        if(node != None):
            self.redCard = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/corners')
        if(node != None):
            self.corners = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/offsides')
        if(node != None):
            self.offsides = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/unadjustedscore')
        if(node != None):
            self.unadjustedScore = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/hasmotm')
        if(node != None):
            self.hasMOTM = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/hascleansheets')
        if(node != None):
            self.hasCleanSheets = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/penaltiesawarded')
        if(node != None):
            self.penaltiesAwarded = node.text              
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/penaltiesscored')
        if(node != None):
            self.penaltiesScored = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/penaltiessaved')
        if(node != None):
            self.penaltiesSaved = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/penaltiesmissed')
        if(node != None):
            self.penaltiesMissed = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/averagepossessionlength')
        if(node != None):
            self.averagePossessionLength = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/passesintercepted')
        if(node != None):
            self.passesIntercepted = node.text              
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/averagefatigueafterninety')
        if(node != None):
            self.averageFatigueAfterNinety = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/commonplayerreport/injuriesred')
        if(node != None):
            self.injuriesRed = node.text        
                

class SSFSeasonPlayerReport:
    ssfCommonStatsReport = 0
    pos = 0
    teamEntityId = 0
    cleanSheetsAny = 0
    cleanSheetsDef = 0
    cleanSheetsGoalKeeper = 0
    manOfTheMatch = 0
    userEndSubReason = 0
    ssfEndResult = 0

    def __init__(self):
        self.pos = ""
        self.teamEntityId = ""
        self.cleanSheetsAny = ""
        self.cleanSheetsDef = ""
        self.cleanSheetsGoalKeeper = ""
        self.manOfTheMatch = ""
        self.userEndSubReason = ""
        self.ssfEndResult = ""
        self.ssfCommonStatsReport = SSFCommonStatsReport()
    
    def PopulateAttributesFromReport(self, xmlContent):
    
        node = xmlContent.find('osdkplayerreport/customplayerreport/pos')
        if(node != None):
            self.pos = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/teamentityid')
        if(node != None):
            self.teamEntityId = node.text    
        node = xmlContent.find('osdkplayerreport/customplayerreport/cleansheetsany')
        if(node != None):
            self.cleanSheetsAny = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/cleansheetsdef')
        if(node != None):
            self.cleanSheetsDef = node.text    
        node = xmlContent.find('osdkplayerreport/customplayerreport/cleansheetsgoalkeeper')
        if(node != None):
            self.cleanSheetsGoalKeeper = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/ssfendresult')
        if(node != None):
            self.ssfEndResult = node.text    
        node = xmlContent.find('osdkplayerreport/customplayerreport/userendsubreason')
        if(node != None):
            self.userEndSubReason = node.text
        node = xmlContent.find('osdkplayerreport/customplayerreport/manofthematch')
        if(node != None):
            self.manOfTheMatch = node.text
         
        SSFCommonStatsReport.PopulateAttributesFromReport(self.ssfCommonStatsReport, xmlContent)
            
class SSFSeasonReportAttribute(BaseMatchAttribute):
    ssfSeasonPlayer = 0        
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.ssfSeasonPlayer = SSFSeasonPlayerReport()    
        
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        SSFSeasonPlayerReport.PopulateAttributesFromReport(self.ssfSeasonPlayer, xmlContent)
 
class SSFSoloReportAttribute(BaseMatchAttribute):
    ssfSeasonPlayer = 0        
    
    def __init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId):
        BaseMatchAttribute.__init__(self, reportStatus, eventTime, skuId, titleId, gameTime, gameReportId, gameType, commonGameReport, personaId)
        self.gameendreason = 0;
        self.ssfSeasonPlayer = SSFSeasonPlayerReport()    
        
    def PopulateAttributesFromReport(self, xmlContent):
        BaseMatchAttribute.PopulateAttributesFromReport(self, xmlContent)
        SSFSeasonPlayerReport.PopulateAttributesFromReport(self.ssfSeasonPlayer, xmlContent)

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

