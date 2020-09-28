#include "stressMapOverride.h"
#include "stressMap.h"

#include <maya/MDagPath.h>
#include <maya/MDrawContext.h>
#include <maya/MFnDagNode.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnMesh.h>
#include <maya/MFrameContext.h>
#include <maya/MHWGeometryUtilities.h>
#include <maya/MPointArray.h>
#include <maya/MUserData.h>
#include <maya/MViewport2Renderer.h>

StressMapOverride::StressMapOverride(const MObject& obj) : MHWRender::MPxDrawOverride(obj, StressMapOverride::draw){}

MUserData* StressMapOverride::prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, const MHWRender::MFrameContext& frameContext, MUserData* data){
  /* --------------------------------------------------------------------------------------------------
  Called before draw, this gives the override a chance to cache any data it needs from the DG and is the only  safe place to do so.

  Paramaters:
  objPath       -MDagPath of the object about to be drawn
  cameraPath    -MDagPath of the camera the object is going to be drawn with
  frameContext  -an MFrameContext containing information about the current frame and render targets etc.
  data          -the old user data or None if this is the first time the override is Called

  Returns:
  an instance of MUserData to be passed to the draw callback and addUIDrawables methodes
  -------------------------------------------------------------------------------------------------- */
  auto* boundsData = dynamic_cast<StressMapUserData*>(data);
  if(!boundsData){
    boundsData = new StressMapUserData();
  }
  MObject obj = objPath.node();
  MPlug stressPlug{obj, StressMap::output};
  MFnDoubleArrayData doubleFn;
  MStatus status;
  status = doubleFn.setObject(stressPlug.asMObject());
  CHECK_MSTATUS(status);

  boundsData -> stressData = doubleFn.array(&status);
  CHECK_MSTATUS(status);

  MFnMesh meshFn;
  MPlug meshPlug{obj, StressMap::inputMesh};
  meshFn.setObject(meshPlug.asMObject());
  meshFn.getPoints(boundsData->meshPoints);

  boundsData->fPath = objPath;

  MFnDagNode dagNodeFn(objPath);
  boundsData -> fBounds = dagNodeFn.boundingBox();

  return boundsData;
}

void StressMapOverride::addUIDrawables(const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext, const MUserData* data){
  /* --------------------------------------------------------------------------------------------------
  Add any UI drawables here

  Paramaters:
  objPath        -MDagPath of the object about to be drawn
  drawManager    -MDrawMnager instance that can be used to add UI drawables
  frameContext   -an MFrameContext containing information about the current frame and render targets etc.
  data           -the user data yhsy was returned by prepareForDraw()

  Returns:
  an instance of MUserData to be passed to the draw callback and addUIDrawables methodes
  -------------------------------------------------------------------------------------------------- */
  const auto* boundsData = dynamic_cast<const StressMapUserData*>(data);
  if(!boundsData){
    return;  //can't draw anything
  }

  const MHWRender::DisplayStatus displayStatus = MHWRender::MGeometryUtilities::displayStatus(objPath);
  const MDoubleArray& stress = boundsData->stressData;
  const MPointArray& points = boundsData->meshPoints;
  const unsigned int stressSize = stress.length();
  const unsigned int pointsSize = points.length();

  //check to make sure the points are all the same size
  if(stressSize != pointsSize) return;

  drawManager.beginDrawable();
  {
    if(frameContext.getDisplayStyle() &
      (MHWRender::MFrameContext::kFlatShaded |
      MHWRender::MFrameContext::kGouraudShaded |
      MHWRender::MFrameContext::kTextured)){
        const unsigned int len = points.length();
        MColor green = MColor(0.0, 1.0f, 0.0, 1.0f);
        MColor red = MColor(1.0f, 0.0, 0.0, 1.0f);
        for(unsigned int i=0; i<pointsSize; ++i){
          MColor color = stress[i] > 0.0f ? stress[i] * green : (-stress[i]) * (red);
          drawManager.setColor(color);
          drawManager.sphere(points[i], 0.2f, 5, 5, true);
        }
      }
  drawManager.endDrawable();
  }
}
