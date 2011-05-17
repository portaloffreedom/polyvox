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

#include "PolyVoxCore/Array.h"
#include "PolyVoxCore/MaterialDensityPair.h"
#include "PolyVoxCore/SurfaceMesh.h"
#include "PolyVoxImpl/MarchingCubesTables.h"
#include "PolyVoxCore/VertexTypes.h"

namespace PolyVox
{
	template< template<typename> class VolumeType, typename VoxelType>
	const uint32_t CubicSurfaceExtractor<VolumeType, VoxelType>::MaxQuadsSharingVertex = 4;

	template< template<typename> class VolumeType, typename VoxelType>
	CubicSurfaceExtractor<VolumeType, VoxelType>::CubicSurfaceExtractor(VolumeType<VoxelType>* volData, Region region, SurfaceMesh<PositionMaterial>* result)
		:m_volData(volData)
		,m_sampVolume(volData)
		,m_regSizeInVoxels(region)
		,m_meshCurrent(result)
	{
		m_meshCurrent->clear();
	}

	template< template<typename> class VolumeType, typename VoxelType>
	void CubicSurfaceExtractor<VolumeType, VoxelType>::execute()
	{
		uint32_t uArrayWidth = m_regSizeInVoxels.getUpperCorner().getX() - m_regSizeInVoxels.getLowerCorner().getX() + 2;
		uint32_t uArrayHeight = m_regSizeInVoxels.getUpperCorner().getY() - m_regSizeInVoxels.getLowerCorner().getY() + 2;

		uint32_t arraySize[3]= {uArrayWidth, uArrayHeight, MaxQuadsSharingVertex};
		m_previousSliceVertices.resize(arraySize);
		m_currentSliceVertices.resize(arraySize);
		memset(m_previousSliceVertices.getRawData(), 0xff, m_previousSliceVertices.getNoOfElements() * sizeof(IndexAndMaterial));
		memset(m_currentSliceVertices.getRawData(), 0xff, m_currentSliceVertices.getNoOfElements() * sizeof(IndexAndMaterial));
		
		
		for(int32_t z = m_regSizeInVoxels.getLowerCorner().getZ(); z <= m_regSizeInVoxels.getUpperCorner().getZ() + 1; z++)
		{
			for(int32_t y = m_regSizeInVoxels.getLowerCorner().getY(); y <= m_regSizeInVoxels.getUpperCorner().getY() + 1; y++)
			{
				for(int32_t x = m_regSizeInVoxels.getLowerCorner().getX(); x <= m_regSizeInVoxels.getUpperCorner().getX() + 1; x++)
				{
					// these are always positive anyway
					uint32_t regX = x - m_regSizeInVoxels.getLowerCorner().getX();
					uint32_t regY = y - m_regSizeInVoxels.getLowerCorner().getY();
					uint32_t regZ = z - m_regSizeInVoxels.getLowerCorner().getZ();

					bool finalX = (x == m_regSizeInVoxels.getUpperCorner().getX() + 1);
					bool finalY = (y == m_regSizeInVoxels.getUpperCorner().getY() + 1);
					bool finalZ = (z == m_regSizeInVoxels.getUpperCorner().getZ() + 1);

					VoxelType currentVoxel = m_volData->getVoxelAt(x,y,z);
					bool currentVoxelIsSolid = currentVoxel.getDensity() >= VoxelType::getThreshold();

					VoxelType negXVoxel = m_volData->getVoxelAt(x-1,y,z);
					bool negXVoxelIsSolid = negXVoxel.getDensity()  >= VoxelType::getThreshold();

					if((currentVoxelIsSolid != negXVoxelIsSolid) && (finalY == false) && (finalZ == false))
					{
						int material = (std::max)(currentVoxel.getMaterial(), negXVoxel.getMaterial());

						/*uint32_t v0 = m_meshCurrent->addVertex(PositionMaterial(Vector3DFloat(regX - 0.5f, regY - 0.5f, regZ - 0.5f), material));
						uint32_t v1 = m_meshCurrent->addVertex(PositionMaterial(Vector3DFloat(regX - 0.5f, regY - 0.5f, regZ + 0.5f), material));
						uint32_t v2 = m_meshCurrent->addVertex(PositionMaterial(Vector3DFloat(regX - 0.5f, regY + 0.5f, regZ - 0.5f), material));
						uint32_t v3 = m_meshCurrent->addVertex(PositionMaterial(Vector3DFloat(regX - 0.5f, regY + 0.5f, regZ + 0.5f), material));*/

						// Check to ensure that when a voxel solid/non-solid change is right on a region border, the vertices are generated on the solid side of the region border
						if(((currentVoxelIsSolid > negXVoxelIsSolid) && finalX == false) || ((currentVoxelIsSolid < negXVoxelIsSolid) && regX != 0))
						{
							uint32_t v0 = addVertex(regX - 0.5f, regY - 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v1 = addVertex(regX - 0.5f, regY - 0.5f, regZ + 0.5f, material, m_currentSliceVertices);
							uint32_t v2 = addVertex(regX - 0.5f, regY + 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v3 = addVertex(regX - 0.5f, regY + 0.5f, regZ + 0.5f, material, m_currentSliceVertices);

							if(currentVoxelIsSolid > negXVoxelIsSolid)
							{
								m_meshCurrent->addTriangleCubic(v0,v1,v2);
								m_meshCurrent->addTriangleCubic(v1,v3,v2);
							}
							else											
							{
								m_meshCurrent->addTriangleCubic(v0,v2,v1);
								m_meshCurrent->addTriangleCubic(v1,v2,v3);
							}
						}
					}

					VoxelType negYVoxel = m_volData->getVoxelAt(x,y-1,z);
					bool negYVoxelIsSolid = negYVoxel.getDensity()  >= VoxelType::getThreshold();

					if((currentVoxelIsSolid != negYVoxelIsSolid) && (finalX == false) && (finalZ == false))
					{
						int material = (std::max)(currentVoxel.getMaterial(),negYVoxel.getMaterial());

						if(((currentVoxelIsSolid > negYVoxelIsSolid) && finalY == false) || ((currentVoxelIsSolid < negYVoxelIsSolid) && regY != 0))
						{
							uint32_t v0 = addVertex(regX - 0.5f, regY - 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v1 = addVertex(regX - 0.5f, regY - 0.5f, regZ + 0.5f, material, m_currentSliceVertices);
							uint32_t v2 = addVertex(regX + 0.5f, regY - 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v3 = addVertex(regX + 0.5f, regY - 0.5f, regZ + 0.5f, material, m_currentSliceVertices);

							if(currentVoxelIsSolid > negYVoxelIsSolid)
							{
								m_meshCurrent->addTriangleCubic(v0,v2,v1);
								m_meshCurrent->addTriangleCubic(v1,v2,v3);
							}
							else
							{
								m_meshCurrent->addTriangleCubic(v0,v1,v2);
								m_meshCurrent->addTriangleCubic(v1,v3,v2);
							}
						}
					}

					VoxelType negZVoxel = m_volData->getVoxelAt(x,y,z-1);
					bool negZVoxelIsSolid = negZVoxel.getDensity()  >= VoxelType::getThreshold();

					if((currentVoxelIsSolid != negZVoxelIsSolid) && (finalX == false) && (finalY == false))
					{
						int material = (std::max)(currentVoxel.getMaterial(), negZVoxel.getMaterial());

						if(((currentVoxelIsSolid > negZVoxelIsSolid) && finalZ == false) || ((currentVoxelIsSolid < negZVoxelIsSolid) && regZ != 0))
						{
							uint32_t v0 = addVertex(regX - 0.5f, regY - 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v1 = addVertex(regX - 0.5f, regY + 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v2 = addVertex(regX + 0.5f, regY - 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
							uint32_t v3 = addVertex(regX + 0.5f, regY + 0.5f, regZ - 0.5f, material, m_previousSliceVertices);
	
							if(currentVoxelIsSolid > negZVoxelIsSolid)
							{
								m_meshCurrent->addTriangleCubic(v0,v1,v2);
								m_meshCurrent->addTriangleCubic(v1,v3,v2);
							}
							else
							{
								m_meshCurrent->addTriangleCubic(v0,v2,v1);
								m_meshCurrent->addTriangleCubic(v1,v2,v3);
							}
						}
					}
				}
			}

			m_previousSliceVertices.swap(m_currentSliceVertices);
			memset(m_currentSliceVertices.getRawData(), 0xff, m_currentSliceVertices.getNoOfElements() * sizeof(IndexAndMaterial));
		}

		m_meshCurrent->m_Region = m_regSizeInVoxels;

		m_meshCurrent->m_vecLodRecords.clear();
		LodRecord lodRecord;
		lodRecord.beginIndex = 0;
		lodRecord.endIndex = m_meshCurrent->getNoOfIndices();
		m_meshCurrent->m_vecLodRecords.push_back(lodRecord);
	}

	template< template<typename> class VolumeType, typename VoxelType>
	int32_t CubicSurfaceExtractor<VolumeType, VoxelType>::addVertex(float fX, float fY, float fZ, uint8_t uMaterialIn, Array<3, IndexAndMaterial>& existingVertices)
	{
		uint32_t uX = static_cast<uint32_t>(fX + 0.75f);
		uint32_t uY = static_cast<uint32_t>(fY + 0.75f);

		for(uint32_t ct = 0; ct < MaxQuadsSharingVertex; ct++)
		{
			IndexAndMaterial& rEntry = existingVertices[uX][uY][ct];

			int32_t iIndex = static_cast<int32_t>(rEntry.iIndex);
			uint8_t uMaterial = static_cast<uint8_t>(rEntry.uMaterial);

			//If we have an existing vertex and the material matches then we can return it.
			if((iIndex != -1) && (uMaterial == uMaterialIn))
			{
				return iIndex;
			}
			else
			{
				//No vertices matched and we've now hit an empty space. Fill it by creating a vertex.
				uint32_t temp = m_meshCurrent->addVertex(PositionMaterial(Vector3DFloat(fX, fY, fZ), uMaterialIn));

				//Note - Slightly dodgy casting taking place here. No proper way to convert to 24-bit int though?
				//If problematic in future then fix IndexAndMaterial to contain variables rather than bitfield.
				rEntry.iIndex = temp;
				rEntry.uMaterial = uMaterialIn;

				return temp;
			}
		}

		//If we exit the loop here then apparently all the slots were full but none of
		//them matched. I don't think this can happen so let's put an assert to make sure.
		assert(false);
		return 0;
	}
}