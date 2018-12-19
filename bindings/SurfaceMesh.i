%module SurfaceMesh
%{
#include "Region.h"
#include "Vertex.h"
#include "Mesh.h"
%}

%include "Region.h"
%include "Vertex.h"
%include "Mesh.h"

//%template(VertexTypeVector) std::vector<PolyVox::VertexType>;
//%template(PositionMaterialVector) std::vector<PolyVox::PositionMaterial>;
//%template(PositionMaterialNormalVector) std::vector<PolyVox::PositionMaterialNormal>;
//%template(LodRecordVector) std::vector<PolyVox::LodRecord>;
//%template(uint8Vector) std::vector<uint8_t>;
//%template(uint32Vector) std::vector<uint32_t>;

// %template(MeshPositionMaterial) PolyVox::Mesh<PolyVox::CubicVertex<uint8_t>, uint16_t >;
// %template(MeshPositionMaterialNormal) PolyVox::Mesh<PolyVox::MarchingCubesVertex<uint8_t>, uint16_t >;

%define DECODERS(volumetype, indextype)
    %template(MeshCubic_ ## volumetype ## _ ## indextype) PolyVox::Mesh<PolyVox::CubicVertex<volumetype>, indextype >;
    %template(MeshMarchingCubes_ ## volumetype ## _ ## indextype) PolyVox::Mesh<PolyVox::MarchingCubesVertex<volumetype>, indextype >;

    %template(decodeMeshCubic_ ## volumetype ## _ ## indextype) PolyVox::decodeMesh<PolyVox::Mesh<PolyVox::CubicVertex<volumetype>, indextype >>;
    %template(decodeMeshMarchingCubes_ ## volumetype ## _ ## indextype) PolyVox::decodeMesh<PolyVox::Mesh<PolyVox::MarchingCubesVertex<volumetype>, indextype >>;

    %template(decodedMeshCubic_ ## volumetype ## _ ## indextype)         PolyVox::Mesh< PolyVox::Vertex< PolyVox::Mesh< PolyVox::CubicVertex< volumetype >,indextype >::VertexType::DataType >,PolyVox::Mesh< PolyVox::CubicVertex< volumetype >,indextype >::IndexType >;
    %template(decodedMeshMarchingCubes_ ## volumetype ## _ ## indextype) PolyVox::Mesh< PolyVox::Vertex< PolyVox::Mesh< PolyVox::MarchingCubesVertex< volumetype >,indextype >::VertexType::DataType >,PolyVox::Mesh< PolyVox::MarchingCubesVertex< volumetype >,indextype >::IndexType >;

    %template(decodedMeshCubicUnit_ ## volumetype ## _ ## indextype)         PolyVox::Vertex< PolyVox::CubicVertex< volumetype >::DataType >;
    %template(decodedMeshMarchingCubesUnit_ ## volumetype ## _ ## indextype) PolyVox::Vertex< PolyVox::MarchingCubesVertex< volumetype >::DataType >;
%enddef

DECODERS(u8, u32)
DECODERS(u16, u32)
DECODERS(u32, u32)
DECODERS(i8, u32)
DECODERS(i16, u32)
DECODERS(i32, u32)
DECODERS(float, u32)
DECODERS(double, u32)
