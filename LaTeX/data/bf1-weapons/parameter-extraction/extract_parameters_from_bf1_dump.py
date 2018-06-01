# Copyright 2018 - Bernhard Rieder - All Rights Reserved.
# Tested with python 3.6.4
# coding: utf-8

import csv
import copy

def readFileLines(fileName):
    lines = []
    with open(fileName, 'r') as file:
        lines = file.read().splitlines()
    return lines

def getParametersToExtract(fileName):
    parametersToExtract = readFileLines(fileName)
    print("Found %i parameters!" % len(parametersToExtract))
    for i in range(0, len(parametersToExtract)):
        parametersToExtract[i] = parametersToExtract[i].lower()
    return parametersToExtract, validateParameters(parametersToExtract)

def validateParameters (parametersToExtract):
    # loop every line
    for line in parametersToExtract:
        if line == "__CustomReload__".lower():
            print("ERROR: Extraction of __CustomReload__ is not supported!")
            return False
    return True

def createEmptyDictEntry(parametersToExtract):
    entry = dict()
    for paramter in parametersToExtract:
        entry[paramter] = "N/A"
    return entry

def extractWeaponsFromDump(dumpFileName, parametersToExtract):
    weapons = []
    weaponParams = {}

    # loop every line
    for line in readFileLines(dumpFileName):
        lineContent = line.strip().split(': ')
        parameter = lineContent[0].lower()

        if len(lineContent) == 1:
            #means this is the header for a weapon
            if len(parameter) > 0:
                weaponParams = createEmptyDictEntry(parametersToExtract)

            # means this is the end of the weapon indicated by a blank entry
            # hint: make sure you have 2 new lines at the end of the file to get all weapons
            elif weaponParams:
                weapons.append(weaponParams)
                weaponParams = {}

        else:
            if parameter not in weaponParams:
                continue

            value = lineContent[1]

            # special case for parameters 'Damages', 'Dmg_distances' and '__CustomReload__' because its in the form of, e.g., [15, 12, 10]
            if "[" in value:
                value = value.replace("[", "").replace("]", "").split(', ')

            weaponParams[parameter] = value

    return weapons

def extractFirstAndLastValueOfParametersWithListValues(weapons):
    parametersWithListValues = []

    #make sure that the all weapon data is constistent afterwards
    for weapon in weapons:
        for parameter, value in weapon.items():
            if isinstance(value, list) and parameter not in parametersWithListValues:
                parametersWithListValues.append(parameter)

    for i in range(0, len(weapons)):
        copy = dict(weapons[i])
        for parameter, value in weapons[i].items():
            if parameter in parametersWithListValues:
                del copy[parameter]
                copy[parameter+"_first"] = value[0]
                copy[parameter+"_last"] = value[len(value)-1]
        weapons[i] = copy
    return weapons

def createAndWriteCSV(weapons, outputFile):
    # write content to csv file
    if len(weapons) > 0:
        with open(outputFile, 'w', newline='') as csvfile:  # Just use 'w' mode in 3.x
            writer = csv.DictWriter(csvfile, weapons[0].keys()) # add delimiter=';' if ',' is not appropriate
            writer.writeheader()
            for line in weapons:
                writer.writerow(line)
        return True
    return False


### MAIN ###
#dumpFileName = "bf1_dump_25_5_2018"
dumpFileName = "../bf1_dump_25_5_2018"
dumpFileType = ".txt"
outputFileName = "extracted_parameters.csv"

parametersToExtract, validParams = getParametersToExtract("parameters_to_extract.txt")
if validParams:
    weapons = extractWeaponsFromDump(dumpFileName+dumpFileType, parametersToExtract)

    print("Found %i weapons!" %len(weapons))

    weapons = extractFirstAndLastValueOfParametersWithListValues(weapons)

    csvSuccess = createAndWriteCSV(weapons, outputFileName)
    if csvSuccess:
        print("Parameter extraction sucessful! Saved in " + outputFileName)
    else:
        print("Error!")
