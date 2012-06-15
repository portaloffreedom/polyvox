/*******************************************************************************
Copyright (c) 2005-2009 David Williams

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

    1. The origin of this software must not be misrepresented; you must not
    claim that you wrote the original software. If you use this software
    in a product, an acknowledgment in the product documentation would be
    appreciated but is not required.

    2. Altered source versions must be plainly marked as such, and must not be
    misrepresented as being the original software.

    3. This notice may not be removed or altered from any source
    distribution. 	
*******************************************************************************/

#include "PolyVoxCore/MaterialDensityPair.h"

namespace PolyVox
{
	template<>
	typename VoxelTypeTraits<MaterialDensityPair44>::DensityType convertToDensity(MaterialDensityPair44 voxel)
	{
		return voxel.getDensity();
	}

	template<>
	typename VoxelTypeTraits<MaterialDensityPair88>::DensityType convertToDensity(MaterialDensityPair88 voxel)
	{
		return voxel.getDensity();
	}

	//template<>
	ConvertToDensity<MaterialDensityPair44>::DensityType ConvertToDensity<MaterialDensityPair44>::operator()(MaterialDensityPair44 voxel)
	{
		return voxel.getDensity();
	}

	//template<>
	ConvertToDensity<MaterialDensityPair88>::DensityType ConvertToDensity<MaterialDensityPair88>::operator()(MaterialDensityPair88 voxel)
	{
		return voxel.getDensity();
	}

	//template<>
	ConvertToMaterial<MaterialDensityPair44>::MaterialType ConvertToMaterial<MaterialDensityPair44>::operator()(MaterialDensityPair44 voxel)
	{
		return voxel.getMaterial();
	}

	//template<>
	ConvertToMaterial<MaterialDensityPair88>::MaterialType ConvertToMaterial<MaterialDensityPair88>::operator()(MaterialDensityPair88 voxel)
	{
		return voxel.getMaterial();
	}
}
