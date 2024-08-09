#include "LandAndWavesApp.h"

#include "Common/GeometryGenerator.h"

void LandAndWavesApp::BuildGeometry() {
    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(10, 10, 10, 5);


}