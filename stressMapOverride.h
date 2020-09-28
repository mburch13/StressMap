#pragma once
#include <maya/MBoundingBox.h>
#include <maya/MPxDrawOverride.h>
#include <maya/MPointArray.h>

//this class can not be iterated from
class StressMapOverride final : public ::MPxDrawOverride{
private:
  class StressMapUserData final : public MUserData{
  public:
    StressMapUserData() : MUserData(false){}
    virtual ~StressMapUserData() = default;

    MDoubleArray stressData;
    MPointArray meshPoints;
    MBoundingBox fBounds;
    MDagPath fPath;
  };

public:
  static MHWRender::MPxDrawOverride* creator(const MObject& obj) {return new StressMapOverride(obj);};

  virtual ~StressMapOverride() = default;

  virtual MHWRender::DrawAPI supportedDrawAPIs() const override { return MHWRender::kAllDevices; };

  //extract all needed data for rendering
  MUserData* prepareForDraw(const MDagPath& objPath, const MDagPath& cameraPath, const MHWRender::MFrameContext& frameContext, MUserData* data) override;

  bool hasUIDrawables() const override {return true;}
  void addUIDrawables(const MDagPath& objPath, MHWRender::MUIDrawManager& drawManager, const MHWRender::MFrameContext& frameContext, const MUserData* data) override;

  static void draw(const MHWRender::MDrawContext& context, const MUserData* data) {};

private:
  //private constructor
  StressMapOverride(const MObject& obj);

};
