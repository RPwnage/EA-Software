from Cheetah.Template import Template
import os
import sys

class AreaCode:
    def __init__(self,area):
        self.area = area
        self.value = 0
        self.comment = None
    
    def __repr__(self):
        return "{0} ({1}) //{2}".format(self.area,self.value,self.comment)
        
        
class ErrorWarning:
    def __init__(self,code):
        self.code = code
        self.comment = None
        self.value = 0
        self.ErrorInfo = []
        
    def _get_hex(self):
        return "0x%0.8X" % (self.value & 0xffffffff)
        
    def _get_area(self):
        return self.ErrorInfo[1].area if len(self.ErrorInfo) >= 2 else "ORIGIN_ERROR_AREA_GENERAL"
        
    area = property(_get_area)
    hex = property(_get_hex)
       
    def __repr__(self):
        return "{0} ({1}) //{2}".format(self.code,self.value,self.comment)
        
def main(input_file,output_dir):
    myfile = open(input_file)
    area_codes = []
    area_code_map = {}
    error_codes = []
    section = 1
    for line in myfile:
        myelements = line.split()
        elements = [element for element in myelements if element not in ("(","+",")")] #don't want these
        
        # First section of file -- Error Areas
        if section == 1:
            if line.startswith("/// $errors"):
                section = 2 #end of this section
                continue
            if len(elements) > 2 and elements[0].startswith("#define"):
                ac = AreaCode(elements[1])
                ac.area = elements[1]
                if elements[2].startswith("0x"): #Value is static
                    ac.value = int(elements[2],16)
                elif "<<" in elements[2]: #Value is a bitshift
                    shifts = elements[2][1:(len(elements[2])-1)].split("<<") #trimming brackets
                    ac.value = int(shifts[0]) << int(shifts[1])
                if len(elements) > 3 and elements[3].startswith("//"): #Comment is available
                    ac.comment = " ".join(elements[4:])
                area_codes.append(ac)
                area_code_map[ac.area] = ac
                
        # Second section of file -- Error Codes
        if section == 2:
            if len(elements) > 2 and elements[0].startswith("#define"):
                ec = ErrorWarning(elements[1])
                for element in elements[2:]:
                    myelement = element.strip("()")
                    if myelement in area_code_map:
                        ec.ErrorInfo.append(area_code_map[myelement])
                        ec.value += area_code_map[myelement].value
                    else:
                        #not an error code, could be a number or we've reached the comment
                        if myelement.startswith("0x"):
                            ec.value += int(myelement,16)
                        elif myelement.startswith("//"):
                            ec.comment = " ".join(elements[elements.index(element):]).strip("/< ")
                            break
                        else:
                            ec.value += int(myelement) #assume number
                error_codes.append(ec)
    
    tmpl = Template(open(os.path.join(os.path.dirname(__file__),"ErrorFileCompiler.tmpl"),'r').read(),searchList=({'error_codes':error_codes,'area_codes':area_codes},))
    parsed_tmpl = str(tmpl)
    my_file = os.path.join(output_dir,"OriginError.cpp")
    if os.path.exists(my_file):
        old = open(my_file,"r").read()
        if old != parsed_tmpl:
            #only write to file if contents have changed
            open(my_file,"w").write(parsed_tmpl)
    else:
        open(my_file,"w").write(parsed_tmpl)    

if __name__ == '__main__':
    if len(sys.argv) != 3:
        print "Usage: [input file] [output dir]"
    else:
        main(sys.argv[1],sys.argv[2])