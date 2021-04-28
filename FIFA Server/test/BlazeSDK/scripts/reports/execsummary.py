import os
import cgi
class cns:
    name = ''
    cs = '0'
    csset = False
    def __init__(self,namein):
        self.name = namein

    def setcs(self,csin):
        self.cs = csin
        self.csset = True
        
    def toString(self):
        return self.name + self.cs
        
def isCond(line):
    if(line.strip().startswith("//")):
        if(line.strip().startswith("// Code size:")):
            return True
        else:
            return False
    if(line.strip().startswith("namespace")):
        return True
    return False

selected = []
namespaces = []
state = 0
inblaze = False

selected = filter(isCond, open("codesizeinfo.txt").readlines())
for line in selected:
    if(inblaze == False):
        if(line.startswith("namespace Blaze")):
            myc = cns("Blaze")
            namespaces.append(myc)
            inblaze = True
            state = 1
            continue
    if(inblaze):
        if(line.find("Code size") != -1):
            if(state == 1):
                myc = namespaces.pop()
                if(myc.csset == False):
                    myc.setcs(line.strip().split(":")[1])
                    namespaces.append(myc)
                state = 0
        elif(line.startswith(" ")):
            if(line.strip().startswith("namespace")):
                myc = cns(line.replace("namespace ","").rstrip("\n"))
                namespaces.append(myc)
                state = 1
        else:
            inblaze = False

summary = open("execsummary.txt","w")
for i in namespaces:
    if(i.cs.strip() != "0"):
        summary.write(i.toString()+"\n")
summary.close()