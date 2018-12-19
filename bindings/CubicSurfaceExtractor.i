%module CubicSurfaceExtractor
%{
#include "CubicSurfaceExtractor.h"
#include "BaseVolume.h"
%}

%include "CubicSurfaceExtractor.h"
%include "BaseVolume.h"

%define CUBICSURFACEEXTRACTORS(volumeclass, volumetype)
    %template(extractCubicMesh_ ## volumeclass ## _ ## volumetype ##) PolyVox::extractCubicMeshDefaultQuad<PolyVox::volumeclass<volumetype>>;
%enddef

CUBICSURFACEEXTRACTORS(RawVolume, u8)
CUBICSURFACEEXTRACTORS(RawVolume, u16)
CUBICSURFACEEXTRACTORS(RawVolume, u32)
CUBICSURFACEEXTRACTORS(RawVolume, i8)
CUBICSURFACEEXTRACTORS(RawVolume, i16)
CUBICSURFACEEXTRACTORS(RawVolume, i32)

//EXTRACTORS(CubicSurfaceExtractor)
