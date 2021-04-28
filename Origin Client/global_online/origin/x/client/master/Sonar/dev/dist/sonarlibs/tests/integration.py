import urllib
import urllib2
import os
import subprocess

#f = open("./sonarsrv.exe", "wb");
#f.write(urllib.urlopen("http://teamcity.services.esn/guestAuth/repository/download/bt324/.lastSuccessful/sonarsrv.exe").read())
#f.close()

os.putenv("SONARSRV_ANSWER_ECHO", "1")
cmds = r"sonarsrv.exe -backendaddress BOGUS:3131 -voipport 0"
serverProcess = subprocess.Popen(cmds, stderr=subprocess.PIPE, stdout=subprocess.PIPE)

def mainMethod():
    line = ""

    while True:
        line = serverProcess.stderr.readline()
        if line.startswith("Public VoIP address:"):
            break
       
    portNumber = int(line.split(":", 3)[2])
 
    print "Port number ", portNumber
 
    os.putenv("SONAR_TEST_VOICE_PORT", str(portNumber))
 
    subprocess.check_call("TestSonarAudio.exe")
    subprocess.check_call("TestSonarInput.exe")
    subprocess.check_call("TestSonarConnection.exe")
    subprocess.check_call("TestSonarClient.exe")
    subprocess.check_call("TestSonarIpc.exe")
    

try:
    mainMethod()
finally:
    serverProcess.terminate()
