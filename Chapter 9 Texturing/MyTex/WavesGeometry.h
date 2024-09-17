#pragma once

#include "Chapter 7 Drawing in Direct3D Part II/LandAndWaves/Waves.h"
#include "Common/d3dUtil.h"
#include "FrameResource.h"

class WavesGeometry {
  public:
    WavesGeometry(int rowCount,
                  int colCount,
                  float timeStep,
                  float spatialStep,
                  float speed,
                  float damping);
    Vertex* VertexData() { return vertices_.data(); }

    unsigned int VertexCount() const { return vertices_.size(); }

    std::uint16_t* IndexData() { return indices_.data(); }

    unsigned int IndexCount() const { return indices_.size(); }

    void Update(float totalTimeFromStart, float deltaTimeSecond);

  private:
    std::vector<Vertex> vertices_;
    std::vector<std::uint16_t> indices_;
    
    Waves waves_;
    float baseTime_ = 0.0f;
};
