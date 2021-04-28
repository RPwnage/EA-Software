import os
import cgi
class cns:
    name = ''
    cs = '0'
    csset = False
    type = 0
    nested = 0
    def __init__(self,namein):
        self.name = namein

    def setcs(self,csin):
        self.cs = csin
        self.csset = True
        
    def toString(self):
        return self.name + self.cs

    def incrNested(self):
        self.nested = self.nested + 1

    def decrNested(self):
        self.nested = self.nested - 1
        
def isCond(line):
    if(line.strip().startswith("//")):
        if(line.strip().startswith("// Code size:")):
            return True
        elif(line.strip().startswith("{")):
            return True
        elif(line.strip().startswith("}")):
            return True
        else:
            return False
    if(line.strip().startswith("namespace")):
        return True
    if(line.strip().startswith("class")):
        return True
    return False

selected = []
namespaces = []
state = 0
inblaze = False
intdf = False

selected = filter(isCond, open("codesizeinfo.txt").readlines())
for line in selected:
    if(inblaze == False):
        if(line.startswith("namespace Blaze")):
            myc = cns("Blaze")
            namespaces.append(myc)
            inblaze = True
            state = 1
            continue
    if(intdf == False):
        if(line.startswith("    namespace TDF")):
            myc = cns("TDF")
            namespaces.append(myc)
            intdf = True
            state = 1
            continue
    if(inblaze or intdf):
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
            elif(line.strip().startswith("class")):
                myc = cns(line.replace("class ","").rstrip("\n"))
                namespaces.append(myc)
                state = 1
            elif(line.strip().startswith("{")):
                myc = namespaces[len(namespaces)-1]
                myc.incrNested()
            elif(line.strip().startswith("}")):
                myc = namespaces[len(namespaces)-1]
                myc.decrNested()
                if (myc.nested == 0):
                    namespaces.pop()
        else:
            inblaze = False
            intdf = False

summary = open("classsummary.txt","w")
for i in namespaces:
    if(i.cs.strip() != "0"):
        summary.write(i.toString()+"\n")
summary.close()
