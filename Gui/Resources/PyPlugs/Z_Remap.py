# -*- coding: utf-8 -*-

#Note that Viewers are never exported
#This file was automatically generated by Natron PyPlug exporter version 1.
import NatronEngine

def getPluginID():
    return "com.coyhot.zRemap.100"

def getLabel():
    return "Z_Remap"

def getVersion():
    return 1

def getGrouping():
    return "Channel"

def getDescription():
    return "Remap Z-Depth pass according a Min/Close value and a Max/Far value."

def createInstance(app,group):

    #Create all nodes in the group
    lastNode = app.createNode("fr.inria.built-in.Output", 1, group)
    lastNode.setScriptName("Output1")
    lastNode.setLabel("Output1")
    lastNode.setPosition(758.75, 300.625)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.6, 0.6, 0.6)
    groupOutput1 = lastNode

    param = lastNode.getParam("Output_layer_name")
    if param is not None:
        param.setValue("RGBA")
        param.setVisible(False)
        del param

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode



    lastNode = app.createNode("fr.inria.built-in.Input", 1, group)
    lastNode.setScriptName("Input1")
    lastNode.setLabel("zDepth")
    lastNode.setPosition(758.75, 29.38)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.300008, 0.500008, 0.2)
    groupInput1 = lastNode

    param = lastNode.getParam("Output_layer_name")
    if param is not None:
        param.setValue("RGBA")
        param.setVisible(False)
        del param

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode



    lastNode = app.createNode("net.sf.openfx.GradePlugin", 1, group)
    lastNode.setScriptName("Grade2")
    lastNode.setLabel("Grade2")
    lastNode.setPosition(756.666, 107.266)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.480003, 0.659998, 1)
    groupGrade2 = lastNode

    param = lastNode.getParam("blackPoint")
    if param is not None:
        param.setValue(100, 0)
        param.setValue(100, 1)
        param.setValue(100, 2)
        del param

    param = lastNode.getParam("whitePoint")
    if param is not None:
        param.setValue(0, 0)
        param.setValue(0, 1)
        param.setValue(0, 2)
        del param

    param = lastNode.getParam("premult")
    if param is not None:
        param.setValue(True)
        del param

    param = lastNode.getParam("premultChannel")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("")
    if param is not None:
        param.setValue("RGBA.A")
        param.setVisible(False)
        del param

    param = lastNode.getParam("Output_layer_name")
    if param is not None:
        param.setValue("RGBA")
        param.setVisible(False)
        del param

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode



    lastNode = app.createNode("net.sf.openfx.Invert", 1, group)
    lastNode.setScriptName("Invert1")
    lastNode.setLabel("Invert1")
    lastNode.setPosition(756.666, 215.299)
    lastNode.setSize(104, 43)
    lastNode.setColor(0.480003, 0.659998, 1)
    groupInvert1 = lastNode

    param = lastNode.getParam("premultChannel")
    if param is not None:
        param.setVisible(False)
        del param

    param = lastNode.getParam("")
    if param is not None:
        param.setValue("RGBA.A")
        param.setVisible(False)
        del param

    param = lastNode.getParam("Output_layer_name")
    if param is not None:
        param.setValue("RGBA")
        param.setVisible(False)
        del param

    param = lastNode.getParam("disableNode")
    if param is not None:
        param.setValue(True)
        del param

    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param

    del lastNode




    #Create the parameters of the group node the same way we did for all internal nodes
    lastNode = group
    param = lastNode.getParam("highDefUpstream")
    if param is not None:
        param.setVisible(False)
        del param


    #Create the user-parameters
    lastNode.userNatron = lastNode.createPageParam("userNatron", "User")
    param = lastNode.createDoubleParam("zremapClose", "Close value")
    param.setMinimum(-2.14748e+09, 0)
    param.setMaximum(2.14748e+09, 0)
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Define the value from the Zdepth you want to set to White.")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.zremapClose = param
    del param

    param = lastNode.createIntParam("zRemapFar", "Far value")
    param.setDisplayMinimum(0, 0)
    param.setDisplayMaximum(100, 0)
    param.setDefaultValue(100, 0)

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Define the value from the Zdepth you want to set to Black.")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.zRemapFar = param
    del param

    param = lastNode.createBooleanParam("zRemapInvert", "Invert gradient")

    #Add the param to the page
    lastNode.userNatron.addParam(param)

    #Set param properties
    param.setHelp("Invert black and white values")
    param.setAddNewLine(True)
    param.setAnimationEnabled(True)
    lastNode.zRemapInvert = param
    del param

    #Refresh the GUI with the newly created parameters
    lastNode.refreshUserParamsGUI()
    del lastNode

    #Now that all nodes are created we can connect them together, restore expressions
    groupOutput1.connectInput(0, groupInvert1)

    groupGrade2.connectInput(0, groupInput1)
    param = groupGrade2.getParam("blackPoint")
    param.setExpression("thisGroup.zRemapFar.get()", False, 0)
    param.setExpression("thisGroup.zRemapFar.get()", False, 1)
    param.setExpression("thisGroup.zRemapFar.get()", False, 2)
    del param
    param = groupGrade2.getParam("whitePoint")
    param.setExpression("thisGroup.zremapClose.get()", False, 0)
    param.setExpression("thisGroup.zremapClose.get()", False, 1)
    param.setExpression("thisGroup.zremapClose.get()", False, 2)
    del param

    groupInvert1.connectInput(0, groupGrade2)
    param = groupInvert1.getParam("disableNode")
    param.setExpression("not(thisGroup.zRemapInvert.get())", False, 0)
    del param

