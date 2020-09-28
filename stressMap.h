//stressMap.h

#ifndef stressMap_H
#define stressMap_H

#include <maya/MTypeId.h>  //Manage Maya Object type identifiers
#include <maya/MPxLocatorNode.h>  //Base class for user defined dependency nodes
#include <maya/MPointArray.h>
#include <maya/MDoubleArray.h>
#include <vector>

//stores all data for a specific point
struct StressPoint{
  MIntArray neighbors; //know all the neighbors
  MDoubleArray neighborsStress;  //know how much the edges changes
  int size;
};

class StressMap final : public MPxLocatorNode{
  public:
    StressMap();
    static MStatus initialize();  //initialize node
    static void* creator() { return new StressMap(); };  //create node
    MStatus compute(const MPlug& plug, MDataBlock& data) override;  //implements core of the node

    void draw(M3dView&, const MDagPath&, M3dView::DisplayStyle, M3dView::DisplayStatus) override;
    bool isBounded() const override { return false; };

    void buildConnectionTree(std::vector<StressPoint>& pointTree, MDoubleArray& stressMapValues, MObject& referenceMesh);

  public:
    //needed variables
    static MTypeId typeId;
    static MObject fakeOut;
    static MObject drawIt;
    static MObject inputMesh;
    static MObject referenceMesh;
    std::vector<StressPoint> pointStoredTree;

    //
    MDoubleArray stressMapValues;
    static const MString kDrawDbClassification;
    static MObject output;
    static MObject multiplier;
    static MObject clampMax;
    static MObject normalize;
    static MObject squashColor;
    static MObject stretchColor;
    static MObject intensity;

    int firstRun;
    MPointArray referencePos;
    MPointArray inputPos;

};

#endif
