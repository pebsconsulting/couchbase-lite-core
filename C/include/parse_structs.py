#!/usr/bin/python

import glob
import re
from datetime import date
import parse_enums

type_map = {"uint32_t":"uint","size_t":"UIntPtr","int32_t":"int","uint8_t":"byte","C4StorageEngine":"string", 
            "char*":"string","uint64_t":"ulong","C4SequenceNumber":"ulong"}
bridge_types = ["UIntPtr","string","bool"]
reverse_bridge_map = {"string":"IntPtr","bool":"byte"}
skip_types = ["C4ReduceFunction","C4FullTextTerm"]
partials = ["C4Error","C4Slice","C4BlobKey","C4EncryptionKey","C4DatabaseConfig","C4IndexOptions","C4EnumeratorOptions","C4QueryOptions"]

def make_property(name, type):
    tin = open("templates/{}.cs".format(type))
    ret_val = tin.read().format(name)
    tin.close()
    return ret_val

if __name__ == "__main__":
    for file in glob.iglob("./*.h"):
        enums = parse_enums.parse_enum(file)
        fin = open(file, "r")
        variables = []
        structs = {}
        in_struct = False
        in_comment = 0
        for line in fin:
            if in_struct:
                if "}" in line:
                    stripped = re.search("\\}\\s*(\\S*?);", line)
                    if stripped.group(1) in skip_types:
                        structs[stripped.group(1)] = ["skip"]
                    else:
                        structs[stripped.group(1)] = variables
                    
                    variables = []
                    in_struct = False
                else:
                    if in_comment > 0:
                        if "*/" in line:
                            in_comment -= 1
                        
                        continue
                    
                    if "/*" in line:
                        in_comment += 1
                        
                    stripped = re.search("([^ ]*?)\\s*(\\*+)?\\s*([^ ]*?);", line)
                    if not stripped:
                        continue
                    
                    if(stripped.group(2)):
                        type = "".join(stripped.group(1,2))
                        name = stripped.group(3)
                    else:
                        type = stripped.group(1)
                        name = stripped.group(3)
                        
                    if type in type_map:
                        type = type_map[type]
                        
                    variables.append(" ".join([type, name]))
            elif re.search("typedef struct.*?{", line):
                in_struct = True
            else:
                opaque = re.search("typedef struct (\\S*) (\\S*);", line)
                if opaque:
                    structs[opaque.group(2)] = []
        
        if len(structs) == 0:
            continue
            
        fout = open(file[2].upper() + file[3:-2] + "_defs.cs", "w");
        out_text = "{}\n\n".format(enums)
        tin = open("templates/header.cs")
        template = tin.read()
        tin.close()
        for name, variables in structs.iteritems():
            properties = []
            if len(variables) == 1 and variables[0] == "skip":
                try:
                    tin = open("templates/{}.cs".format(name))
                except:
                    print "No definition found for {}; skipping...".format(name)
                    continue
                    
                out_text += "{}\n\n".format(tin.read())
                tin.close()
                continue
            
            partial = "partial " if name in partials else ""
            out_text += "    public unsafe {}struct {}\n    {{\n".format(partial, name)
            for variable in variables:
                arg_info = variable.split()
                name = arg_info[1]
                type = arg_info[0]
                modifier = "public"
                fixed = ""
                for bridge_type in bridge_types:
                    if(type == bridge_type):  
                        properties.append(make_property(name, type))
                        name = "_" + name
                        modifier = "private"
                        if type in reverse_bridge_map:
                            type = reverse_bridge_map[type]
                        break
                        
                if(re.search("\\[\\d+\\]", name)):
                    fixed = "fixed "
                            
                out_text += "        {} {}{} {};\n".format(modifier, fixed, type, name)
            
            if len(properties) > 0:
                out_text += "\n"
                out_text += "\n\n".join(properties)
                out_text += "\n"
                
            out_text += "    }\n\n"
                
        final_text = template % {"filename":file[2].upper() + file[3:-2] + "_defs.cs","year":date.today().year,"structs":out_text[:-2]}
        fout.write(final_text)
        fout.close()        
        
