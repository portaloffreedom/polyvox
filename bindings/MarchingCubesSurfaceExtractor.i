%module MarchingCubesSurfaceExtractor
%{
#include "MarchingCubesSurfaceExtractor.h"
%}

%include "MarchingCubesSurfaceExtractor.h"

%define MARCHINGCUBESSURFACEEXTRACTORS(volumeclass, volumetype)
    %template(extractMarchingCubesMesh_ ## volumeclass ## _ ## volumetype ##) PolyVox::extractMarchingCubesMeshDefaultController<PolyVox::volumeclass<volumetype>>;
%enddef

MARCHINGCUBESSURFACEEXTRACTORS(RawVolume, u8)
MARCHINGCUBESSURFACEEXTRACTORS(RawVolume, u16)
MARCHINGCUBESSURFACEEXTRACTORS(RawVolume, u32)
MARCHINGCUBESSURFACEEXTRACTORS(RawVolume, i8)
MARCHINGCUBESSURFACEEXTRACTORS(RawVolume, i16)
MARCHINGCUBESSURFACEEXTRACTORS(RawVolume, i32)

//EXTRACTORS(MarchingCubesSurfaceExtractor)
