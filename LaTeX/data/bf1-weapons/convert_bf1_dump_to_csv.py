# Copyright 2018 - Bernhard Rieder - All Rights Reserved.
# Tested with python 3.6.4
# coding: utf-8

import csv

dump_file_name = "bf1_dump_25_5_2018"
dump_file_type = ".txt"
output_file = dump_file_name + "_converted.csv"

print("Start converting file '" + dump_file_name + "' to '" + output_file + "'")

fileContent = []
numOfEntries = 0

# open the dump file
with open(dump_file_name+dump_file_type) as dump_file:
    dump_data = dump_file.readlines() 
    
    # create an empty dict to add dump entry content
    entry = {}
    
    # loop every line 
    for line in dump_data:
        lineContent = line.strip().split(': ')
                
        # get and write the content of this line in our entry dict
        if len(lineContent) >= 2:
            if lineContent[0] == "__CustomReload__":
                continue
            # special case for damage because its listed with damage over distance -> e.g. [15, 12, 10] ...
            if "[" in lineContent[1]:
                items = lineContent[1].replace("[", "").replace("]", "").split(', ')
                
                # change the number if you want more entries
                for x in range(0,10):
                    entry[lineContent[0]+str(x)] = items[x] if x < len(items) else items[len(items)-1]
                    
            # otherwise just write the current content as usual 
            else:
                entry[lineContent[0]] = lineContent[1]
                
        # fires only if there is no content in this line and the entry has something in it
        # it maybe useful to add 2 new lines at the end of the file!
        elif entry:
                fileContent.append(entry) # save the current entry
                entry = {} # and create a new one so that's empty
                numOfEntries +=1
                
                
# write content to csv file
if len(fileContent) > 0:
    with open(output_file, 'w', newline='') as csvfile:  # Just use 'w' mode in 3.x
        writer = csv.DictWriter(csvfile, fileContent[0].keys()) # add delimiter=';' if ',' is not appropriate 
        writer.writeheader()
        for line in fileContent:
            writer.writerow(line)

print("Conversion sucessful! Found " + str(numOfEntries) + " entries.")

    
        

