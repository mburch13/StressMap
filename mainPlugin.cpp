//McKenzie Burch
//Rigging Dojo Maya API
//mainPlugin.cpp
//standard setup for event node

#include "stressMap.h"  //change include header for each node
#include "stressMapOverride.h"
#include <maya/MDrawRegistry.h>
#include <maya/MGlobal.h>
#include <maya/MFnPlugin.h> //Maya class that redisters and deregisters plug-ins with Maya

const MString pluginRegistrantId("stressMap");

MStatus initializePlugin(MObject obj){
  MStatus status;
  MFnPlugin fnplugin(obj, "McKenzie Burch", "1.0", "Any");

  status = fnplugin.registerNode("stressMap", StressMap::typeId, StressMap::creator, StressMap::initialize, MPxNode::kLocatorNode, &StressMap::kDrawDbClassification);

  if(status != MS::kSuccess){
    status.perror("Could not regiser the stressMap node");
    return status;
  }

  //register the box handle draw override
  status = MHWRender::MDrawRegistry::registerDrawOverrideCreator(StressMap::kDrawDbClassification, pluginRegistrantId, StressMapOverride::creator);

  if(!status){
    MGlobal::displayError("Undable to register stressMap draw override.");
    return status;
  }

  return status;
}

MStatus uninitializePlugin(MObject obj){
  MFnPlugin pluginFn;
  //deregister the given user defined node type Maya
  pluginFn.deregisterNode(StressMap::typeId);

  MStatus status = MHWRender::MDrawRegistry::deregisterDrawOverrideCreator(StressMap::kDrawDbClassification, pluginRegistrantId);

  if(!status){
    MGlobal::displayError("Unable to register stressMap draw override");
    return status;
  }

  return MS::kSuccess;
}
