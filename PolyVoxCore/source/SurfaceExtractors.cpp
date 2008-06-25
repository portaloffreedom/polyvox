#include "SurfaceExtractors.h"

#include "BlockVolume.h"
#include "GradientEstimators.h"
#include "IndexedSurfacePatch.h"
#include "MarchingCubesTables.h"
#include "Region.h"
#include "RegionGeometry.h"
#include "SurfaceAdjusters.h"
#include "SurfaceExtractorsDecimated.h"
#include "VolumeChangeTracker.h"
#include "BlockVolumeIterator.h"

#include <algorithm>

using namespace std;

namespace PolyVox
{
	std::list<RegionGeometry> getChangedRegionGeometry(VolumeChangeTracker& volume)
	{
		std::list<Region> listChangedRegions;
		volume.getChangedRegions(listChangedRegions);

		std::list<RegionGeometry> listChangedRegionGeometry;
		for(std::list<Region>::const_iterator iterChangedRegions = listChangedRegions.begin(); iterChangedRegions != listChangedRegions.end(); ++iterChangedRegions)
		{
			//Generate the surface
			RegionGeometry regionGeometry;
			regionGeometry.m_patchSingleMaterial = new IndexedSurfacePatch(false);
			regionGeometry.m_v3dRegionPosition = iterChangedRegions->getLowerCorner();

			generateDecimatedMeshDataForRegion(volume.getVolumeData(), 0, *iterChangedRegions, regionGeometry.m_patchSingleMaterial);

			//generateReferenceMeshDataForRegion(volume.getVolumeData(), *iterChangedRegions, regionGeometry.m_patchSingleMaterial);
		
			//for(int ct = 0; ct < 2; ct++)
			Vector3DInt32 temp = regionGeometry.m_v3dRegionPosition;
			//temp /= 16;
			if(temp.getX() % 32 == 0)
			{
				smoothRegionGeometry(volume.getVolumeData(), regionGeometry);
			}

			//computeNormalsForVertices(volume.getVolumeData(), regionGeometry, CENTRAL_DIFFERENCE);

			//genMultiFromSingle(regionGeometry.m_patchSingleMaterial, regionGeometry.m_patchMultiMaterial);

			regionGeometry.m_bContainsSingleMaterialPatch = regionGeometry.m_patchSingleMaterial->getVertices().size() > 0;
			regionGeometry.m_bIsEmpty = (regionGeometry.m_patchSingleMaterial->getVertices().size() == 0) || (regionGeometry.m_patchSingleMaterial->getIndices().size() == 0);

			listChangedRegionGeometry.push_back(regionGeometry);
		}

		return listChangedRegionGeometry;
	}

	std::uint32_t getIndex(std::uint32_t x, std::uint32_t y)
	{
		return x + (y * (POLYVOX_REGION_SIDE_LENGTH+1));
	}

	void generateRoughMeshDataForRegion(BlockVolume<uint8_t>* volumeData, Region region, IndexedSurfacePatch* singleMaterialPatch)
	{	
		singleMaterialPatch->m_vecVertices.clear();
		singleMaterialPatch->m_vecTriangleIndices.clear();

		//For edge indices
		std::int32_t* vertexIndicesX0 = new std::int32_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];
		std::int32_t* vertexIndicesY0 = new std::int32_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];
		std::int32_t* vertexIndicesZ0 = new std::int32_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];
		std::int32_t* vertexIndicesX1 = new std::int32_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];
		std::int32_t* vertexIndicesY1 = new std::int32_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];
		std::int32_t* vertexIndicesZ1 = new std::int32_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];

		//Cell bitmasks
		std::uint8_t* bitmask0 = new std::uint8_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];
		std::uint8_t* bitmask1 = new std::uint8_t[(POLYVOX_REGION_SIDE_LENGTH+1) * (POLYVOX_REGION_SIDE_LENGTH+1)];

		//When generating the mesh for a region we actually look one voxel outside it in the
		// back, bottom, right direction. Protect against access violations by cropping region here
		Region regVolume = volumeData->getEnclosingRegion();
		regVolume.setUpperCorner(regVolume.getUpperCorner() - Vector3DInt32(1,1,1));
		region.cropTo(regVolume);

		//Offset from volume corner
		const Vector3DFloat offset = static_cast<Vector3DFloat>(region.getLowerCorner());

		//Create a region corresponding to the first slice
		Region regSlice0(region);
		regSlice0.setUpperCorner(Vector3DInt32(regSlice0.getUpperCorner().getX(),regSlice0.getUpperCorner().getY(),regSlice0.getLowerCorner().getZ()));
		
		//Iterator to access the volume data
		BlockVolumeIterator<std::uint8_t> volIter(*volumeData);		

		//Compute bitmask for initial slice
		std::uint32_t uNoOfNonEmptyCellsForSlice0 = computeInitialRoughBitmaskForSlice(volIter, regSlice0, offset, bitmask0);		
		if(uNoOfNonEmptyCellsForSlice0 != 0)
		{
			//If there were some non-empty cells then generate initial slice vertices for them
			generateRoughVerticesForSlice(volIter,regSlice0, offset, bitmask0, singleMaterialPatch, vertexIndicesX0, vertexIndicesY0, vertexIndicesZ0);
		}

		for(std::uint32_t uSlice = 0; ((uSlice <= POLYVOX_REGION_SIDE_LENGTH-1) && (uSlice + offset.getZ() < region.getUpperCorner().getZ())); ++uSlice)
		{
			Region regSlice1(regSlice0);
			regSlice1.shift(Vector3DInt32(0,0,1));

			std::uint32_t uNoOfNonEmptyCellsForSlice1 = computeRoughBitmaskForSliceFromPrevious(volIter, regSlice1, offset, bitmask1, bitmask0);

			if(uNoOfNonEmptyCellsForSlice1 != 0)
			{
				generateRoughVerticesForSlice(volIter,regSlice1, offset, bitmask1, singleMaterialPatch, vertexIndicesX1, vertexIndicesY1, vertexIndicesZ1);				
			}

			if((uNoOfNonEmptyCellsForSlice0 != 0) || (uNoOfNonEmptyCellsForSlice1 != 0))
			{
				generateRoughIndicesForSlice(volIter, regSlice0, singleMaterialPatch, offset, bitmask0, bitmask1, vertexIndicesX0, vertexIndicesY0, vertexIndicesZ0, vertexIndicesX1, vertexIndicesY1, vertexIndicesZ1);
			}

			std::swap(uNoOfNonEmptyCellsForSlice0, uNoOfNonEmptyCellsForSlice1);
			std::swap(bitmask0, bitmask1);
			std::swap(vertexIndicesX0, vertexIndicesX1);
			std::swap(vertexIndicesY0, vertexIndicesY1);
			std::swap(vertexIndicesZ0, vertexIndicesZ1);

			regSlice0 = regSlice1;
		}

		delete[] bitmask0;
		delete[] bitmask1;
		delete[] vertexIndicesX0;
		delete[] vertexIndicesX1;
		delete[] vertexIndicesY0;
		delete[] vertexIndicesY1;
		delete[] vertexIndicesZ0;
		delete[] vertexIndicesZ1;
	}

	std::uint32_t computeInitialRoughBitmaskForSlice(BlockVolumeIterator<uint8_t>& volIter, const Region& regSlice, const Vector3DFloat& offset, uint8_t* bitmask)
	{
		std::uint32_t uNoOfNonEmptyCells = 0;

		//Iterate over each cell in the region
		volIter.setPosition(regSlice.getLowerCorner().getX(),regSlice.getLowerCorner().getY(), regSlice.getLowerCorner().getZ());
		volIter.setValidRegion(regSlice);
		do
		{		
			//Current position
			const uint16_t x = volIter.getPosX() - offset.getX();
			const uint16_t y = volIter.getPosY() - offset.getY();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = 0;

			if((x==0) && (y==0))
			{
				const uint8_t v000 = volIter.getVoxel();
				const uint8_t v100 = volIter.peekVoxel1px0py0pz();
				const uint8_t v010 = volIter.peekVoxel0px1py0pz();
				const uint8_t v110 = volIter.peekVoxel1px1py0pz();

				const uint8_t v001 = volIter.peekVoxel0px0py1pz();
				const uint8_t v101 = volIter.peekVoxel1px0py1pz();
				const uint8_t v011 = volIter.peekVoxel0px1py1pz();
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();			

				if (v000 == 0) iCubeIndex |= 1;
				if (v100 == 0) iCubeIndex |= 2;
				if (v110 == 0) iCubeIndex |= 4;
				if (v010 == 0) iCubeIndex |= 8;
				if (v001 == 0) iCubeIndex |= 16;
				if (v101 == 0) iCubeIndex |= 32;
				if (v111 == 0) iCubeIndex |= 64;
				if (v011 == 0) iCubeIndex |= 128;
			}
			else if((x>0) && y==0)
			{
				const uint8_t v100 = volIter.peekVoxel1px0py0pz();
				const uint8_t v110 = volIter.peekVoxel1px1py0pz();

				const uint8_t v101 = volIter.peekVoxel1px0py1pz();
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();			

				//x
				uint8_t iPreviousCubeIndexX = bitmask[getIndex(x-1,y)];
				uint8_t srcBit6 = iPreviousCubeIndexX & 64;
				uint8_t destBit7 = srcBit6 << 1;
				
				uint8_t srcBit5 = iPreviousCubeIndexX & 32;
				uint8_t destBit4 = srcBit5 >> 1;

				uint8_t srcBit2 = iPreviousCubeIndexX & 4;
				uint8_t destBit3 = srcBit2 << 1;
				
				uint8_t srcBit1 = iPreviousCubeIndexX & 2;
				uint8_t destBit0 = srcBit1 >> 1;

				iCubeIndex |= destBit0;
				if (v100 == 0) iCubeIndex |= 2;
				if (v110 == 0) iCubeIndex |= 4;
				iCubeIndex |= destBit3;
				iCubeIndex |= destBit4;
				if (v101 == 0) iCubeIndex |= 32;
				if (v111 == 0) iCubeIndex |= 64;
				iCubeIndex |= destBit7;
			}
			else if((x==0) && (y>0))
			{
				const uint8_t v010 = volIter.peekVoxel0px1py0pz();
				const uint8_t v110 = volIter.peekVoxel1px1py0pz();

				const uint8_t v011 = volIter.peekVoxel0px1py1pz();
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();

				//y
				uint8_t iPreviousCubeIndexY = bitmask[getIndex(x,y-1)];
				uint8_t srcBit7 = iPreviousCubeIndexY & 128;
				uint8_t destBit4 = srcBit7 >> 3;
				
				uint8_t srcBit6 = iPreviousCubeIndexY & 64;
				uint8_t destBit5 = srcBit6 >> 1;

				uint8_t srcBit3 = iPreviousCubeIndexY & 8;
				uint8_t destBit0 = srcBit3 >> 3;
				
				uint8_t srcBit2 = iPreviousCubeIndexY & 4;
				uint8_t destBit1 = srcBit2 >> 1;

				iCubeIndex |= destBit0;
				iCubeIndex |= destBit1;
				if (v110 == 0) iCubeIndex |= 4;
				if (v010 == 0) iCubeIndex |= 8;
				iCubeIndex |= destBit4;
				iCubeIndex |= destBit5;
				if (v111 == 0) iCubeIndex |= 64;
				if (v011 == 0) iCubeIndex |= 128;
			}
			else
			{
				const uint8_t v110 = volIter.peekVoxel1px1py0pz();

				const uint8_t v111 = volIter.peekVoxel1px1py1pz();

				//y
				uint8_t iPreviousCubeIndexY = bitmask[getIndex(x,y-1)];
				uint8_t srcBit7 = iPreviousCubeIndexY & 128;
				uint8_t destBit4 = srcBit7 >> 3;
				
				uint8_t srcBit6 = iPreviousCubeIndexY & 64;
				uint8_t destBit5 = srcBit6 >> 1;

				uint8_t srcBit3 = iPreviousCubeIndexY & 8;
				uint8_t destBit0 = srcBit3 >> 3;
				
				uint8_t srcBit2 = iPreviousCubeIndexY & 4;
				uint8_t destBit1 = srcBit2 >> 1;

				//x
				uint8_t iPreviousCubeIndexX = bitmask[getIndex(x-1,y)];
				srcBit6 = iPreviousCubeIndexX & 64;
				uint8_t destBit7 = srcBit6 << 1;

				srcBit2 = iPreviousCubeIndexX & 4;
				uint8_t destBit3 = srcBit2 << 1;

				iCubeIndex |= destBit0;
				iCubeIndex |= destBit1;
				if (v110 == 0) iCubeIndex |= 4;
				iCubeIndex |= destBit3;
				iCubeIndex |= destBit4;
				iCubeIndex |= destBit5;
				if (v111 == 0) iCubeIndex |= 64;
				iCubeIndex |= destBit7;
			}

			//Save the bitmask
			bitmask[getIndex(x,y)] = iCubeIndex;

			if(edgeTable[iCubeIndex] != 0)
			{
				++uNoOfNonEmptyCells;
			}
			
		}while(volIter.moveForwardInRegionXYZ());//For each cell

		return uNoOfNonEmptyCells;
	}

	std::uint32_t computeRoughBitmaskForSliceFromPrevious(BlockVolumeIterator<uint8_t>& volIter, const Region& regSlice, const Vector3DFloat& offset, uint8_t* bitmask, uint8_t* previousBitmask)
	{
		std::uint32_t uNoOfNonEmptyCells = 0;

		//Iterate over each cell in the region
		volIter.setPosition(regSlice.getLowerCorner().getX(),regSlice.getLowerCorner().getY(), regSlice.getLowerCorner().getZ());
		volIter.setValidRegion(regSlice);
		do
		{		
			//Current position
			const uint16_t x = volIter.getPosX() - offset.getX();
			const uint16_t y = volIter.getPosY() - offset.getY();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = 0;

			if((x==0) && (y==0))
			{
				const uint8_t v001 = volIter.peekVoxel0px0py1pz();
				const uint8_t v101 = volIter.peekVoxel1px0py1pz();
				const uint8_t v011 = volIter.peekVoxel0px1py1pz();
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();			

				//z
				uint8_t iPreviousCubeIndexZ = previousBitmask[getIndex(x,y)];
				iCubeIndex = iPreviousCubeIndexZ >> 4;

				if (v001 == 0) iCubeIndex |= 16;
				if (v101 == 0) iCubeIndex |= 32;
				if (v111 == 0) iCubeIndex |= 64;
				if (v011 == 0) iCubeIndex |= 128;
			}
			else if((x>0) && y==0)
			{
				const uint8_t v101 = volIter.peekVoxel1px0py1pz();
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();			

				//z
				uint8_t iPreviousCubeIndexZ = previousBitmask[getIndex(x,y)];
				iCubeIndex = iPreviousCubeIndexZ >> 4;

				//x
				uint8_t iPreviousCubeIndexX = bitmask[getIndex(x-1,y)];
				uint8_t srcBit6 = iPreviousCubeIndexX & 64;
				uint8_t destBit7 = srcBit6 << 1;
				
				uint8_t srcBit5 = iPreviousCubeIndexX & 32;
				uint8_t destBit4 = srcBit5 >> 1;

				iCubeIndex |= destBit4;
				if (v101 == 0) iCubeIndex |= 32;
				if (v111 == 0) iCubeIndex |= 64;
				iCubeIndex |= destBit7;
			}
			else if((x==0) && (y>0))
			{
				const uint8_t v011 = volIter.peekVoxel0px1py1pz();
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();

				//z
				uint8_t iPreviousCubeIndexZ = previousBitmask[getIndex(x,y)];
				iCubeIndex = iPreviousCubeIndexZ >> 4;

				//y
				uint8_t iPreviousCubeIndexY = bitmask[getIndex(x,y-1)];
				uint8_t srcBit7 = iPreviousCubeIndexY & 128;
				uint8_t destBit4 = srcBit7 >> 3;
				
				uint8_t srcBit6 = iPreviousCubeIndexY & 64;
				uint8_t destBit5 = srcBit6 >> 1;

				iCubeIndex |= destBit4;
				iCubeIndex |= destBit5;
				if (v111 == 0) iCubeIndex |= 64;
				if (v011 == 0) iCubeIndex |= 128;
			}
			else
			{
				const uint8_t v111 = volIter.peekVoxel1px1py1pz();			

				//z
				uint8_t iPreviousCubeIndexZ = previousBitmask[getIndex(x,y)];
				iCubeIndex = iPreviousCubeIndexZ >> 4;

				//y
				uint8_t iPreviousCubeIndexY = bitmask[getIndex(x,y-1)];
				uint8_t srcBit7 = iPreviousCubeIndexY & 128;
				uint8_t destBit4 = srcBit7 >> 3;
				
				uint8_t srcBit6 = iPreviousCubeIndexY & 64;
				uint8_t destBit5 = srcBit6 >> 1;

				//x
				uint8_t iPreviousCubeIndexX = bitmask[getIndex(x-1,y)];
				srcBit6 = iPreviousCubeIndexX & 64;
				uint8_t destBit7 = srcBit6 << 1;

				iCubeIndex |= destBit4;
				iCubeIndex |= destBit5;
				if (v111 == 0) iCubeIndex |= 64;
				iCubeIndex |= destBit7;
			}

			//Save the bitmask
			bitmask[getIndex(x,y)] = iCubeIndex;

			if(edgeTable[iCubeIndex] != 0)
			{
				++uNoOfNonEmptyCells;
			}
			
		}while(volIter.moveForwardInRegionXYZ());//For each cell

		return uNoOfNonEmptyCells;
	}

	void generateRoughVerticesForSlice(BlockVolumeIterator<uint8_t>& volIter, Region& regSlice, const Vector3DFloat& offset, uint8_t* bitmask, IndexedSurfacePatch* singleMaterialPatch,std::int32_t vertexIndicesX[],std::int32_t vertexIndicesY[],std::int32_t vertexIndicesZ[])
	{
		//Iterate over each cell in the region
		volIter.setPosition(regSlice.getLowerCorner().getX(),regSlice.getLowerCorner().getY(), regSlice.getLowerCorner().getZ());
		volIter.setValidRegion(regSlice);
		//while(volIter.moveForwardInRegionXYZ())
		do
		{		
			//Current position
			const uint16_t x = volIter.getPosX() - offset.getX();
			const uint16_t y = volIter.getPosY() - offset.getY();
			const uint16_t z = volIter.getPosZ() - offset.getZ();

			const uint8_t v000 = volIter.getVoxel();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = bitmask[getIndex(x,y)];

			/* Cube is entirely in/out of the surface */
			if (edgeTable[iCubeIndex] == 0)
			{
				continue;
			}

			/* Find the vertices where the surface intersects the cube */
			if (edgeTable[iCubeIndex] & 1)
			{
				if((x + offset.getX()) != regSlice.getUpperCorner().getX())
				{
					const uint8_t v100 = volIter.peekVoxel1px0py0pz();
					const Vector3DFloat v3dPosition(x + 0.5f, y, z);
					const Vector3DFloat v3dNormal(v000 > v100 ? 1.0f : -1.0f, 0.0f, 0.0f);					
					const uint8_t uMaterial = v000 | v100; //Because one of these is 0, the or operation takes the max.
					const SurfaceVertex surfaceVertex(v3dPosition, v3dNormal, uMaterial, 1.0);
					singleMaterialPatch->m_vecVertices.push_back(surfaceVertex);
					vertexIndicesX[getIndex(x,y)] = singleMaterialPatch->m_vecVertices.size()-1;
				}
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				if((y + offset.getY()) != regSlice.getUpperCorner().getY())
				{
					const uint8_t v010 = volIter.peekVoxel0px1py0pz();
					const Vector3DFloat v3dPosition(x, y + 0.5f, z);
					const Vector3DFloat v3dNormal(0.0f, v000 > v010 ? 1.0f : -1.0f, 0.0f);
					const uint8_t uMaterial = v000 | v010;
					SurfaceVertex surfaceVertex(v3dPosition, v3dNormal, uMaterial, 1.0);
					singleMaterialPatch->m_vecVertices.push_back(surfaceVertex);
					vertexIndicesY[getIndex(x,y)] = singleMaterialPatch->m_vecVertices.size()-1;
				}
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				//if((z + offset.getZ()) != upperCorner.getZ())
				{
					const uint8_t v001 = volIter.peekVoxel0px0py1pz();
					const Vector3DFloat v3dPosition(x, y, z + 0.5f);
					const Vector3DFloat v3dNormal(0.0f, 0.0f, v000 > v001 ? 1.0f : -1.0f);
					const uint8_t uMaterial = v000 | v001;
					SurfaceVertex surfaceVertex(v3dPosition, v3dNormal, uMaterial, 1.0);
					singleMaterialPatch->m_vecVertices.push_back(surfaceVertex);
					vertexIndicesZ[getIndex(x,y)] = singleMaterialPatch->m_vecVertices.size()-1;
				}
			}
		}while(volIter.moveForwardInRegionXYZ());//For each cell
	}

	void generateRoughIndicesForSlice(BlockVolumeIterator<uint8_t>& volIter, const Region& regSlice, IndexedSurfacePatch* singleMaterialPatch, const Vector3DFloat& offset, uint8_t* bitmask0, uint8_t* bitmask1, std::int32_t vertexIndicesX0[],std::int32_t vertexIndicesY0[],std::int32_t vertexIndicesZ0[], std::int32_t vertexIndicesX1[],std::int32_t vertexIndicesY1[],std::int32_t vertexIndicesZ1[])
	{
		std::uint32_t indlist[12];

		Region regCroppedSlice(regSlice);		
		regCroppedSlice.setUpperCorner(regCroppedSlice.getUpperCorner() - Vector3DInt32(1,1,0));

		volIter.setPosition(regCroppedSlice.getLowerCorner().getX(),regCroppedSlice.getLowerCorner().getY(), regCroppedSlice.getLowerCorner().getZ());
		volIter.setValidRegion(regCroppedSlice);
		do
		{		
			//Current position
			const uint16_t x = volIter.getPosX() - offset.getX();
			const uint16_t y = volIter.getPosY() - offset.getY();
			const uint16_t z = volIter.getPosZ() - offset.getZ();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = bitmask0[getIndex(x,y)];

			/* Cube is entirely in/out of the surface */
			if (edgeTable[iCubeIndex] == 0)
			{
				continue;
			}

			/* Find the vertices where the surface intersects the cube */
			if (edgeTable[iCubeIndex] & 1)
			{
				indlist[0] = vertexIndicesX0[getIndex(x,y)];
				assert(indlist[0] != -1);
			}
			if (edgeTable[iCubeIndex] & 2)
			{
				indlist[1] = vertexIndicesY0[getIndex(x+1,y)];
				assert(indlist[1] != -1);
			}
			if (edgeTable[iCubeIndex] & 4)
			{
				indlist[2] = vertexIndicesX0[getIndex(x,y+1)];
				assert(indlist[2] != -1);
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				indlist[3] = vertexIndicesY0[getIndex(x,y)];
				assert(indlist[3] != -1);
			}
			if (edgeTable[iCubeIndex] & 16)
			{
				indlist[4] = vertexIndicesX1[getIndex(x,y)];
				assert(indlist[4] != -1);
			}
			if (edgeTable[iCubeIndex] & 32)
			{
				indlist[5] = vertexIndicesY1[getIndex(x+1,y)];
				assert(indlist[5] != -1);
			}
			if (edgeTable[iCubeIndex] & 64)
			{
				indlist[6] = vertexIndicesX1[getIndex(x,y+1)];
				assert(indlist[6] != -1);
			}
			if (edgeTable[iCubeIndex] & 128)
			{
				indlist[7] = vertexIndicesY1[getIndex(x,y)];
				assert(indlist[7] != -1);
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				indlist[8] = vertexIndicesZ0[getIndex(x,y)];
				assert(indlist[8] != -1);
			}
			if (edgeTable[iCubeIndex] & 512)
			{
				indlist[9] = vertexIndicesZ0[getIndex(x+1,y)];
				assert(indlist[9] != -1);
			}
			if (edgeTable[iCubeIndex] & 1024)
			{
				indlist[10] = vertexIndicesZ0[getIndex(x+1,y+1)];
				assert(indlist[10] != -1);
			}
			if (edgeTable[iCubeIndex] & 2048)
			{
				indlist[11] = vertexIndicesZ0[getIndex(x,y+1)];
				assert(indlist[11] != -1);
			}

			for (int i=0;triTable[iCubeIndex][i]!=-1;i+=3)
			{
				std::uint32_t ind0 = indlist[triTable[iCubeIndex][i  ]];
				std::uint32_t ind1 = indlist[triTable[iCubeIndex][i+1]];
				std::uint32_t ind2 = indlist[triTable[iCubeIndex][i+2]];

				singleMaterialPatch->m_vecTriangleIndices.push_back(ind0);
				singleMaterialPatch->m_vecTriangleIndices.push_back(ind1);
				singleMaterialPatch->m_vecTriangleIndices.push_back(ind2);
			}//For each triangle
		}while(volIter.moveForwardInRegionXYZ());//For each cell
	}

	void generateReferenceMeshDataForRegion(BlockVolume<uint8_t>* volumeData, Region region, IndexedSurfacePatch* singleMaterialPatch)
	{	
		//When generating the mesh for a region we actually look one voxel outside it in the
		// back, bottom, right direction. Protect against access violations by cropping region here
		Region regVolume = volumeData->getEnclosingRegion();
		//regVolume.setUpperCorner(regVolume.getUpperCorner() - Vector3DInt32(1,1,1));
		region.cropTo(regVolume);
		region.setUpperCorner(region.getUpperCorner() - Vector3DInt32(1,1,1));

		//Offset from lower block corner
		const Vector3DFloat offset = static_cast<Vector3DFloat>(region.getLowerCorner());

		Vector3DFloat vertlist[12];
		uint8_t vertMaterials[12];
		BlockVolumeIterator<std::uint8_t> volIter(*volumeData);
		volIter.setValidRegion(region);

		//////////////////////////////////////////////////////////////////////////
		//Get mesh data
		//////////////////////////////////////////////////////////////////////////

		//Iterate over each cell in the region
		volIter.setPosition(region.getLowerCorner().getX(),region.getLowerCorner().getY(), region.getLowerCorner().getZ());
		while(volIter.moveForwardInRegionXYZ())
		{		
			//Current position
			const uint16_t x = volIter.getPosX();
			const uint16_t y = volIter.getPosY();
			const uint16_t z = volIter.getPosZ();

			//Voxels values
			const uint8_t v000 = volIter.getVoxel();
			const uint8_t v100 = volIter.peekVoxel1px0py0pz();
			const uint8_t v010 = volIter.peekVoxel0px1py0pz();
			const uint8_t v110 = volIter.peekVoxel1px1py0pz();
			const uint8_t v001 = volIter.peekVoxel0px0py1pz();
			const uint8_t v101 = volIter.peekVoxel1px0py1pz();
			const uint8_t v011 = volIter.peekVoxel0px1py1pz();
			const uint8_t v111 = volIter.peekVoxel1px1py1pz();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uint8_t iCubeIndex = 0;

			if (v000 == 0) iCubeIndex |= 1;
			if (v100 == 0) iCubeIndex |= 2;
			if (v110 == 0) iCubeIndex |= 4;
			if (v010 == 0) iCubeIndex |= 8;
			if (v001 == 0) iCubeIndex |= 16;
			if (v101 == 0) iCubeIndex |= 32;
			if (v111 == 0) iCubeIndex |= 64;
			if (v011 == 0) iCubeIndex |= 128;

			/* Cube is entirely in/out of the surface */
			if (edgeTable[iCubeIndex] == 0)
			{
				continue;
			}

			/* Find the vertices where the surface intersects the cube */
			if (edgeTable[iCubeIndex] & 1)
			{
				vertlist[0].setX(x + 0.5f);
				vertlist[0].setY(y);
				vertlist[0].setZ(z);
				vertMaterials[0] = v000 | v100; //Because one of these is 0, the or operation takes the max.
			}
			if (edgeTable[iCubeIndex] & 2)
			{
				vertlist[1].setX(x + 1.0f);
				vertlist[1].setY(y + 0.5f);
				vertlist[1].setZ(z);
				vertMaterials[1] = v100 | v110;
			}
			if (edgeTable[iCubeIndex] & 4)
			{
				vertlist[2].setX(x + 0.5f);
				vertlist[2].setY(y + 1.0f);
				vertlist[2].setZ(z);
				vertMaterials[2] = v010 | v110;
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				vertlist[3].setX(x);
				vertlist[3].setY(y + 0.5f);
				vertlist[3].setZ(z);
				vertMaterials[3] = v000 | v010;
			}
			if (edgeTable[iCubeIndex] & 16)
			{
				vertlist[4].setX(x + 0.5f);
				vertlist[4].setY(y);
				vertlist[4].setZ(z + 1.0f);
				vertMaterials[4] = v001 | v101;
			}
			if (edgeTable[iCubeIndex] & 32)
			{
				vertlist[5].setX(x + 1.0f);
				vertlist[5].setY(y + 0.5f);
				vertlist[5].setZ(z + 1.0f);
				vertMaterials[5] = v101 | v111;
			}
			if (edgeTable[iCubeIndex] & 64)
			{
				vertlist[6].setX(x + 0.5f);
				vertlist[6].setY(y + 1.0f);
				vertlist[6].setZ(z + 1.0f);
				vertMaterials[6] = v011 | v111;
			}
			if (edgeTable[iCubeIndex] & 128)
			{
				vertlist[7].setX(x);
				vertlist[7].setY(y + 0.5f);
				vertlist[7].setZ(z + 1.0f);
				vertMaterials[7] = v001 | v011;
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				vertlist[8].setX(x);
				vertlist[8].setY(y);
				vertlist[8].setZ(z + 0.5f);
				vertMaterials[8] = v000 | v001;
			}
			if (edgeTable[iCubeIndex] & 512)
			{
				vertlist[9].setX(x + 1.0f);
				vertlist[9].setY(y);
				vertlist[9].setZ(z + 0.5f);
				vertMaterials[9] = v100 | v101;
			}
			if (edgeTable[iCubeIndex] & 1024)
			{
				vertlist[10].setX(x + 1.0f);
				vertlist[10].setY(y + 1.0f);
				vertlist[10].setZ(z + 0.5f);
				vertMaterials[10] = v110 | v111;
			}
			if (edgeTable[iCubeIndex] & 2048)
			{
				vertlist[11].setX(x);
				vertlist[11].setY(y + 1.0f);
				vertlist[11].setZ(z + 0.5f);
				vertMaterials[11] = v010 | v011;
			}

			for (int i=0;triTable[iCubeIndex][i]!=-1;i+=3)
			{
				//The three vertices forming a triangle
				const Vector3DFloat vertex0 = vertlist[triTable[iCubeIndex][i  ]] - offset;
				const Vector3DFloat vertex1 = vertlist[triTable[iCubeIndex][i+1]] - offset;
				const Vector3DFloat vertex2 = vertlist[triTable[iCubeIndex][i+2]] - offset;

				//Cast to floats and divide by two.
				//const Vector3DFloat vertex0AsFloat = (static_cast<Vector3DFloat>(vertex0) / 2.0f) - offset;
				//const Vector3DFloat vertex1AsFloat = (static_cast<Vector3DFloat>(vertex1) / 2.0f) - offset;
				//const Vector3DFloat vertex2AsFloat = (static_cast<Vector3DFloat>(vertex2) / 2.0f) - offset;

				const uint8_t material0 = vertMaterials[triTable[iCubeIndex][i  ]];
				const uint8_t material1 = vertMaterials[triTable[iCubeIndex][i+1]];
				const uint8_t material2 = vertMaterials[triTable[iCubeIndex][i+2]];


				//If all the materials are the same, we just need one triangle for that material with all the alphas set high.
				SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1f,1.0f);
				SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1f,1.0f);
				SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1f,1.0f);
				singleMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
			}//For each triangle
		}//For each cell

		//FIXME - can it happen that we have no vertices or triangles? Should exit early?


		//for(std::map<uint8_t, IndexedSurfacePatch*>::iterator iterPatch = surfacePatchMapResult.begin(); iterPatch != surfacePatchMapResult.end(); ++iterPatch)
		/*{

			std::vector<SurfaceVertex>::iterator iterSurfaceVertex = singleMaterialPatch->getVertices().begin();
			while(iterSurfaceVertex != singleMaterialPatch->getVertices().end())
			{
				Vector3DFloat tempNormal = computeNormal(volumeData, static_cast<Vector3DFloat>(iterSurfaceVertex->getPosition() + offset), CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}
		}*/
	}
}