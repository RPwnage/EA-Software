import sys
import string
import os

def DoCommand0(command):
    print(command)
    os.system(command)
    
def dumpSample(inputFilename, outputFilename):    
    output = open (outputFilename, 'w')

    #hex dumping the input to a C header file
    sArrayVar = string.replace(inputFilename, ".", "_")
    output.write("#ifndef " + sArrayVar + "_H\n")
    output.write("#define " + sArrayVar + "_H\n\n")
    
    output.write("namespace Blaze{namespace GameReporting{\n")
    
    output.write("unsigned char " + sArrayVar + "[] = {\n")

    input = open (inputFilename, 'rb') 
    hex_byte = input.read(1).encode('hex')
    byte_wrote = 0
    while hex_byte != "":
        if (byte_wrote == 0):
            output.write("0x")
        else:
            output.write(",0x")
        output.write(hex_byte)
        hex_byte = input.read(1).encode('hex')
        byte_wrote=byte_wrote+1
        
    input.close()

    output.write("\n};\n")
    output.write("int32_t ")
    output.write(sArrayVar)
    output.write("_len = ")
    output.write(str(byte_wrote))
    output.write(";\n\n")
    output.write("}}\n")
    output.write("#endif\n")

    output.close()

def syncAndProcess(filename, syncPath, outFile, doSync):
    print "Sampling " + filename    
    
    if (doSync == "true"):
        print "syncing at " +  syncPath
        DoCommand0("p4 sync -f "+ syncPath + filename)
    
    dumpSample(filename, outFile)    
 
if(sys.argv[1] == "PRE"):
    #check if the file exists, if not sync perforce    
    outputPath = sys.argv[2]
    doSync = sys.argv[3] 
    syncPath = sys.argv[4]
    
    outFile = outputPath + "sampleLarge.h"
    syncAndProcess("SampleData_large.bin", syncPath, outFile, doSync)

    outFile = outputPath + "sampleSmall.h"
    syncAndProcess("SampleData_small.bin", syncPath, outFile, doSync)

    outFile = outputPath + "sampleTiny.h"
    syncAndProcess("SampleData_tiny.bin", syncPath, outFile, doSync)
elif(sys.argv[1] =="POST"):
    #check if the file exists, if not sync perforce    
    outputPath = sys.argv[2]
    syncPath = sys.argv[3]

    outFile = outputPath + "sampleLarge.h"
    os.remove(outFile)   
    
    outFile = outputPath + "sampleSmall.h"
    os.remove(outFile)
    
    outFile = outputPath + "sampleTiny.h"
    os.remove(outFile)
    
    if(sys.argv[3] == "true"):
        DoCommand0("p4 sync "+ syncPath + "SampleData_large.bin#none")
        
    




