//stressMap.cpp
#  include <OpenGL/gl.h>  // definitions for GL graphics routines
#  include <OpenGL/glu.h> // definitions for GL input device handling
#  include <GLUT/glut.h>  // deginitions for the GLUT utility toolkit

#include "stressMap.h"

#include <maya/MFnCompoundAttribute.h>
#include <maya/MFnDoubleArrayData.h>
#include <maya/MFnEnumAttribute.h>
#include <maya/MFnMatrixAttribute.h>
#include <maya/MFnMesh.h>
#include <maya/MFnNumericAttribute.h>  //numeric attribute function set
#include <maya/MFnTypedAttribute.h>  //static class provviding common API global functions
#include <maya/MGlobal.h>  //static class provviding common API global functions
#include <maya/MIntArray.h>
#include <maya/MItMeshPolygon.h>
#include <maya/MItMeshVertex.h>
#include <maya/MPlug.h>
#include <maya/MPointArray.h>
#include <maya/MVector.h>
#include <maya/MViewport2Renderer.h>

MTypeId StressMap::typeId(0x9011E000);  //define value for typeId

//node drawdb classification string
const MString StressMap::kDrawDbClassification("drawdb/geometry/stressMapDO");

MObject StressMap::fakeOut;
MObject StressMap::drawIt;
MObject StressMap::inputMesh;
MObject StressMap::referenceMesh;
MObject StressMap::output;
MObject StressMap::multiplier;
MObject StressMap::clampMax;
MObject StressMap::normalize;
MObject StressMap::squashColor;
MObject StressMap::stretchColor;
MObject StressMap::intensity;

StressMap::StressMap() : firstRun(0){ }

MStatus StressMap::initialize(){
  MFnEnumAttribute enumFn;
  MFnMatrixAttribute matrixFn;
  MFnNumericAttribute numFn;
  MFnCompoundAttribute compA;
  MFnTypedAttribute typedFn;

  inputMesh = typedFn.create("inputMesh", "inm", MFnData::kMesh);
  addAttribute(inputMesh);

  referenceMesh = typedFn.create("referenceMesh", "rfm", MFnData::kMesh);
  typedFn.setStorable(true);
  addAttribute(referenceMesh);

  drawIt = numFn.create("drawIt", "drw", MFnNumericData::kBoolean, 1);
  numFn.setStorable(true);
  numFn.setKeyable(true);
  addAttribute(drawIt);

  clampMax = numFn.create("clampMax", "cma", MFnNumericData::kDouble, 1);
  numFn.setKeyable(true);
  numFn.setStorable(true);
  numFn.setMin(0);
  numFn.setMax(10);
  addAttribute(clampMax);

  multiplier = numFn.create("nultiplier", "multiplier", MFnNumericData::kDouble, 1);
  numFn.setStorable(true);
  numFn.setKeyable(true);
  addAttribute(multiplier);

  normalize = numFn.create("normalize", "nor", MFnNumericData::kBoolean, 0);
  numFn.setKeyable(true);
  numFn.setStorable(true);
  addAttribute(normalize);

  squashColor = numFn.createColor("squashColor", "sqc");
  numFn.setDefault(0.0f, 1.0f, 0.0f);
  numFn.setKeyable(true);
  numFn.setStorable(true);
  numFn.setReadable(true);
  numFn.setWritable(true);
  addAttribute(squashColor);

  stretchColor = numFn.createColor("stretchColor", "stc");
  numFn.setDefault(1.0f, 0.0f, 0.0f);
  numFn.setStorable(true);
  numFn.setKeyable(true);
  numFn.setReadable(true);
  numFn.setWritable(true);
  addAttribute(stretchColor);

  intensity = numFn.create("intensity", "ity", MFnNumericData::kDouble, 1);
  numFn.setStorable(true);
  numFn.setKeyable(true);
  addAttribute(intensity);

  //used to force maya to evaluate
  //connect to locator -> visibility
  fakeOut = numFn.create("fakeOut", "fo", MFnNumericData::kBoolean, 1);
  numFn.setKeyable(true);
  numFn.setStorable(true);
  addAttribute(fakeOut);

  output = typedFn.create("output", "out", MFnData::kDoubleArray);
  typedFn.setKeyable(false);
  typedFn.setWritable(false);
  typedFn.setStorable(false);
  addAttribute(output);

  attributeAffects(inputMesh, output);
  attributeAffects(referenceMesh, output);
  attributeAffects(clampMax, output);
  attributeAffects(multiplier, output);
  attributeAffects(squashColor, output);
  attributeAffects(stretchColor, output);
  attributeAffects(intensity, output);
  attributeAffects(normalize, output);

  attributeAffects(inputMesh, fakeOut);
  attributeAffects(referenceMesh, fakeOut);
  attributeAffects(clampMax, fakeOut);
  attributeAffects(multiplier, fakeOut);
  attributeAffects(squashColor, fakeOut);
  attributeAffects(stretchColor, fakeOut);
  attributeAffects(intensity, fakeOut);
  attributeAffects(normalize, fakeOut); //video has output not fakeOut??

  //Attribute Editor
  MString stressTemplateNode(MString() + "global proc AEstressMapTemplate( string $nodeName)\n" +
    "{editorTemplate -beginScrollLayout;\n" +
    "editorTemplate -beginLayout \"Setting Attributes\" -collapse 0;\n" +
    "editorTemplate -addControl \"normalize\";\n" +
    "editorTemplate -addControl \"clampMax\";\n" +
    "editorTemplate -addControl \"multiplier\";\n" +
    "editorTemplate -endLayout;\n" +

    "editorTemplate -beginLayout \"Drawing Attributes\" -collapse 0;\n" +
    "editorTemplate -addControl \"drawIt\";\n" +
    "editorTemplate -addControl \"intensity\";\n" +
    "editorTemplate -addControl \"squashColor\";\n" +
    "editorTemplate -addControl \"stretchColor\";\n" +
    "editorTemplate -endLayout;\n" +

    "editorTemplate -addExtraControls;\n" +
    "editorTemplate -endScrollLayout;\n}");

  MGlobal::executeCommand(stressTemplateNode);

  return MS::kSuccess;
}

MStatus StressMap::compute(const MPlug& plug, MDataBlock& dataBlock){
  //trigger only when needed
  MPlug inputMeshP(thisMObject(), inputMesh);
  if(!inputMeshP.isConnected()){
    return MS::kNotImplemented;
  }

  MPlug referenceMeshP(thisMObject(), referenceMesh);
  if(!referenceMeshP.isConnected()){
    return MS::kNotImplemented;
  }

  //gather data
  MObject referenceMeshV = dataBlock.inputValue(referenceMesh).asMesh();
  MObject inputMeshV = dataBlock.inputValue(inputMesh).asMesh();
  const double multiplierV = dataBlock.inputValue(multiplier).asDouble();
  const double clampItMaxV = dataBlock.inputValue(clampMax).asDouble();
  const bool normalizeV = dataBlock.inputValue(normalize).asBool();

  //build tree if needed
  if ((firstRun == 0) || (pointStoredTree.empty() == 1) || (stressMapValues.length() == 0)){
    buildConnectionTree(pointStoredTree, stressMapValues, referenceMeshV);
  }

  //get input points
  MFnMesh inMeshFn(inputMeshV);
  inMeshFn.getPoints(inputPos, MSpace::kObject);

  //get reference points
  MFnMesh refMeshFn(referenceMeshV);
  refMeshFn.getPoints(referencePos, MSpace::kObject);

  const unsigned int intLength = inputPos.length();

  //check input point size
  if(intLength != referencePos.length()){
    MGlobal::displayError("Mismatching point number between input mesh and reference mesh");
    return MS::kSuccess;
  }

  //check if size of stored point is the same of the inPoints
  if(pointStoredTree.size() != intLength){
    MGlobal::displayError("Mismatching in the ref and main data try to rebuild");
    return MS::kSuccess;
  }

  double value = 0;
  MVector storedLen, currentLen;

  //loop every vertex to calculate stress data
  for(int v=0; v<intLength; v++){
    value = 0;

    //lets loop all the connected vtxs of the current vertex
    for(int n=0; n<pointStoredTree[v].size; n++){
      int connIndex = pointStoredTree[v].neighbors[n];  //alias for the neighbors for better readability

      //get vector length
      storedLen = MVector(referencePos[connIndex] - referencePos[v]);
      currentLen = MVector(inputPos[connIndex] - inputPos[v]);

      //accumulate
      value += (currentLen.length() / storedLen.length());
    } //end of n loop

    //average the full value by the number of edges
    value = value / static_cast<double>(pointStoredTree[v].size);

    value -= 1;  //remap the value from 0 to 2 range to -1 to 1 range
    value *= multiplierV;  //multiply value by the multiplier

    if (normalizeV == 1 && value > clampItMaxV) value = clampItMaxV;  //clamp the value

    stressMapValues[v] = value;
  }

  //set the output data
  MFnDoubleArrayData outDataFn;
  MObject outData = outDataFn.create(stressMapValues);
  dataBlock.outputValue(output).setMObject(outData);
  dataBlock.outputValue(output).setClean();

  dataBlock.outputValue(fakeOut).set(0);
  dataBlock.outputValue(fakeOut).setClean();

  return MS::kSuccess;
}

inline void stressLine(MPoint& p, float stress, const float* squashColor, const float* stretchColor, const float mult){
  //check if the stress1 is greater than 0, if so this means its stretched
  const float* color = stress > 0 ? stretchColor : squashColor;
  //simply clamping the stretch in a range we like
  stress = stress > 1.0f ? 1.0f : stress;
  stress = stress <= -0.95f ? 0.95f : stress;
  //check wheather we got a compression or stretch, if is a compressing the
  //value will be negative so we need to flip the sign
  stress = stress < 0.0f ? -stress : stress;
  //alt way stress = std::fabs(stress); may sometimes be cast to double then float again

  //setting the color and adding the vertex
  glColor4f(color[0] * stress * mult, color[1] * stress * mult, color[2] * stress * mult, 1.0);
  glVertex3d(p.x, p.y, p.z);
}

void StressMap::draw(M3dView& view, const MDagPath& path, M3dView::DisplayStyle dispStyle, M3dView::DisplayStatus status){
  MPlug drawItP(thisMObject(), drawIt);
  bool drawItV;
  drawItP.getValue(drawItV);

  if(drawItV == 0){
    return;
  }

  //this is just for forcing the output tp evaluate
  MPlug fakeP(thisMObject(), fakeOut);
  bool fakeV;
  fakeP.getValue(fakeV);
  MPlug outP(thisMObject(), output);
  MObject outV;
  outP.getValue(outV);

  //get input mesh
  MPlug inputMeshP(thisMObject(), inputMesh);
  MObject inputMeshV;
  inputMeshP.getValue(inputMeshV);

  MFnMesh meshFn(inputMeshV);
  MPointArray inPoint;
  meshFn.getPoints(inPoint);
  MItMeshPolygon faceIt(inputMeshV);

  //get colors
  MPlug plug(thisMObject(), squashColor);
  MObject object;
  plug.getValue(object);
  MFnNumericData fn(object);

  float squashColorV[] = {0,0,0,1};
  float stretchColorV[] = {0,0,0,1};
  fn.getData(squashColorV[0], squashColorV[1], squashColorV[2]);

  MPlug plug2(thisMObject(), stretchColor);
  fn.setObject(plug2.asMObject());
  fn.getData(stretchColorV[0], stretchColorV[1], stretchColorV[2]);

  //color mult
  MPlug intensityP(thisMObject(), intensity);
  const float intensityVf = intensityP.asFloat();

  if(stressMapValues.length() != inPoint.length()){
    return;
  }

  /*----------------------------------------------------------------
  Initialize OpenGl and draw
  ----------------------------------------------------------------*/
  view.beginGL();
  glPushAttrib(GL_ALL_ATTRIB_BITS);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glLineWidth(2);
  if(status == M3dView::kLead)
    glColor4f(0.0, 1.0, 0.0, 1.0f);
  else
    glColor4f(1.0, 1.0, 0.0, 1.0f);

  faceIt.reset();

  std::vector<int> edgesDone;
  int size = meshFn.numEdges();
  edgesDone.resize(size);

  //this can be optimized with a memset to 0 -> goint to be much faster
  for(int i=0; i<size; i++){
    edgesDone[i] = 0;
  }

  MIntArray facePoint;
  MIntArray edges;

  //start the draw of the line
  glBegin(GL_LINES);
  for(; faceIt.isDone() != 1; faceIt.next()){
    //draw the edges
    //get vertexs
    faceIt.getEdges(edges);
    faceIt.getVertices(facePoint);

    int len = edges.length();
    for(int e=0; e<len; e++){
      //check if edges were not done already
      if(edgesDone[edges[e]] == 0){
        //edges are in the same order of the points given back to us
        //means that the first edge will have points index point[0] and point[1]

        //here we check if we are not at the last edge we use point n and n+1
        int vtx1 = facePoint[e];
        int vtx2 = e != (len - 1) ? facePoint[e+1] : facePoint[0];

        //draw the points
        stressLine(inPoint[vtx1], stressMapValues[vtx1], squashColorV, stretchColorV, intensityVf);
        stressLine(inPoint[vtx2], stressMapValues[vtx2], squashColorV, stretchColorV, intensityVf);

        //mark the edge as drawn
        edgesDone[edges[e]] = 1;
      } //end of if edgesDone
    } //end for e loop
  } //end for faceIt
  glEnd();
  glDisable(GL_BLEND);
  glPopAttrib();
}

void StressMap::buildConnectionTree(std::vector<StressPoint>& pointTree, MDoubleArray& stressMapValues, MObject& referenceMesh){
  pointTree.clear();  //clear array and free the memory
  stressMapValues.clear();  //clear the stress values

  //init mesh functions
  MItMeshVertex iter(referenceMesh);
  MFnMesh meshFn(referenceMesh);
  MPointArray points;
  meshFn.getPoints(points);

  int size = points.length();
  //allocate memory for the arrays
  pointTree.resize(size);
  stressMapValues.setLength(size);

  //needed variables
  int oldIndex;

  for(unsigned int i = 0; i<points.length(); i++){
    StressPoint pnt;

    iter.setIndex(i, oldIndex);
    MIntArray vtxs;
    iter.getConnectedVertices(vtxs);
    pnt.neighbors = vtxs;
    pnt.size = vtxs.length();
    pointTree[i] = pnt;
    stressMapValues[i] = 0;
  }
}
