/******************************************************************************
This file is part of a voxel plugin for OGRE
Copyright (C) 2006  David Williams

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
******************************************************************************/

#include "MarchingCubesTables.h"
#include "MaterialMapManager.h"
#include "SurfaceVertex.h"
#include "SurfaceEdge.h"
#include "IndexedSurfacePatch.h"
#include "PolyVoxSceneManager.h"
#include "VolumeIterator.h"
#include "VolumeManager.h"

#include "OgreStringConverter.h"
#include "OgreLogManager.h"

#include <list>

namespace Ogre
{

	//////////////////////////////////////////////////////////////////////////
	// PolyVoxSceneManagerFactory
	//////////////////////////////////////////////////////////////////////////
	const String PolyVoxSceneManagerFactory::FACTORY_TYPE_NAME = "PolyVoxSceneManager";

	SceneManager* PolyVoxSceneManagerFactory::createInstance(
		const String& instanceName)
	{
		return new PolyVoxSceneManager(instanceName);
	}

	void PolyVoxSceneManagerFactory::destroyInstance(SceneManager* instance)
	{
		delete instance;
	}

	void PolyVoxSceneManagerFactory::initMetaData(void) const
	{
		mMetaData.typeName = FACTORY_TYPE_NAME;
		mMetaData.description = "A voxel based scene manager";
		mMetaData.sceneTypeMask = ST_GENERIC;
		mMetaData.worldGeometrySupported = false;
	}

	//////////////////////////////////////////////////////////////////////////
	// PolyVoxSceneManager
	//////////////////////////////////////////////////////////////////////////
	PolyVoxSceneManager::PolyVoxSceneManager(const String& name)
		: SceneManager(name)
		,volumeData(0)
		,useNormalSmoothing(false)
		,normalSmoothingFilterSize(1)
		,m_normalGenerationMethod(SOBEL)
		,m_bHaveGeneratedMeshes(false)
		,m_axisNode(0)
	{	
		//sceneNodes.clear();
	}

	PolyVoxSceneManager::~PolyVoxSceneManager()
	{
	}

	const String& PolyVoxSceneManager::getTypeName(void) const
	{
		return PolyVoxSceneManagerFactory::FACTORY_TYPE_NAME;
	}

	bool PolyVoxSceneManager::loadScene(const String& filename)
	{
		volumeData = VolumeManager::getSingletonPtr()->load(filename + ".volume", "General");
		if(volumeData.isNull())
		{
			LogManager::getSingleton().logMessage("Generating default volume");
			generateLevelVolume();
			LogManager::getSingleton().logMessage("Done generating default volume");
		}

		volumeData->tidy();

		//Load material map
		materialMap = MaterialMapManager::getSingletonPtr()->load(filename + ".materialmap", "General");

		setAllUpToDateFlagsTo(false);

		getRootSceneNode()->removeAndDestroyAllChildren();

		createAxis(256);
		setAxisVisible(false);




		return true;
	}

	std::list<RegionGeometry> PolyVoxSceneManager::getChangedRegionGeometry(void)
	{
		std::list<RegionGeometry> listChangedRegionGeometry;

		//Regenerate meshes.
		for(uint regionZ = 0; regionZ < OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS; ++regionZ)
		{		
			//LogManager::getSingleton().logMessage("regionZ = " + StringConverter::toString(regionZ));
			for(uint regionY = 0; regionY < OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS; ++regionY)
			{
				//LogManager::getSingleton().logMessage("regionY = " + StringConverter::toString(regionY));
				for(uint regionX = 0; regionX < OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS; ++regionX)
				{
					//LogManager::getSingleton().logMessage("regionX = " + StringConverter::toString(regionX));
					if(surfaceUpToDate[regionX][regionY][regionZ] == false)
					{
						//Generate the surface
						RegionGeometry regionGeometry;
						regionGeometry.m_patchSingleMaterial = new IndexedSurfacePatch(false);
						regionGeometry.m_patchMultiMaterial = new IndexedSurfacePatch(true);
						regionGeometry.m_v3dRegionPosition.setData(regionX, regionY, regionZ);

						generateMeshDataForRegion(regionX,regionY,regionZ, regionGeometry.m_patchSingleMaterial, regionGeometry.m_patchMultiMaterial);

						regionGeometry.m_bContainsSingleMaterialPatch = regionGeometry.m_patchSingleMaterial->m_vecVertices.size() > 0;
						regionGeometry.m_bContainsMultiMaterialPatch = regionGeometry.m_patchMultiMaterial->m_vecVertices.size() > 0;
						regionGeometry.m_bIsEmpty = ((regionGeometry.m_patchSingleMaterial->m_vecVertices.size() == 0) && (regionGeometry.m_patchMultiMaterial->m_vecTriangleIndices.size() == 0));

						listChangedRegionGeometry.push_back(regionGeometry);
					}
				}
			}
		}

		return listChangedRegionGeometry;
	}

	void PolyVoxSceneManager::setAllUpToDateFlagsTo(bool newUpToDateValue)
	{
		for(uint blockZ = 0; blockZ < OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS; ++blockZ)
		{
			for(uint blockY = 0; blockY < OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS; ++blockY)
			{
				for(uint blockX = 0; blockX < OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS; ++blockX)
				{
					surfaceUpToDate[blockX][blockY][blockZ] = newUpToDateValue;
				}
			}
		}
	}

	void PolyVoxSceneManager::createSphereAt(Vector3 centre, Real radius, uchar value, bool painting)
	{
		int firstX = static_cast<int>(std::floor(centre.x - radius));
		int firstY = static_cast<int>(std::floor(centre.y - radius));
		int firstZ = static_cast<int>(std::floor(centre.z - radius));

		int lastX = static_cast<int>(std::ceil(centre.x + radius));
		int lastY = static_cast<int>(std::ceil(centre.y + radius));
		int lastZ = static_cast<int>(std::ceil(centre.z + radius));

		Real radiusSquared = radius * radius;

		//Check bounds
		firstX = std::max(firstX,0);
		firstY = std::max(firstY,0);
		firstZ = std::max(firstZ,0);

		lastX = std::min(lastX,int(OGRE_VOLUME_SIDE_LENGTH-1));
		lastY = std::min(lastY,int(OGRE_VOLUME_SIDE_LENGTH-1));
		lastZ = std::min(lastZ,int(OGRE_VOLUME_SIDE_LENGTH-1));

		VolumeIterator volIter(*volumeData);
		volIter.setValidRegion(firstX,firstY,firstZ,lastX,lastY,lastZ);
		volIter.setPosition(firstX,firstY,firstZ);
		while(volIter.isValidForRegion())
		{
			//if((volIter.getPosX()*volIter.getPosX()+volIter.getPosY()*volIter.getPosY()+volIter.getPosZ()*volIter.getPosZ()) < radiusSquared)
			if((centre - Vector3(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ())).squaredLength() <= radiusSquared)
			{
				if(painting)
				{
					if(volIter.getVoxel() != 0)
					{
						volIter.setVoxel(value);
						//volIter.setVoxelAt(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ(),value);
					}
				}
				else
				{
					volIter.setVoxel(value);
					//volIter.setVoxelAt(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ(),value);
				}
				//markVoxelChanged(volIter.getPosX(),volIter.getPosY(),volIter.getPosZ()); //FIXME - create a version of this function to mark larger regions at a time.
			}
			volIter.moveForwardInRegion();
		}
		markRegionChanged(firstX,firstY,firstZ,lastX,lastY,lastZ);
	}

	void PolyVoxSceneManager::generateLevelVolume(void)
	{
		volumeData = VolumePtr(new Volume);
		VolumeIterator volIter(*volumeData);
		for(uint z = 0; z < OGRE_VOLUME_SIDE_LENGTH; ++z)
		{
			for(uint y = 0; y < OGRE_VOLUME_SIDE_LENGTH; ++y)
			{
				for(uint x = 0; x < OGRE_VOLUME_SIDE_LENGTH; ++x)
				{
					if((x/16+y/16+z/16)%2 == 0)
						volIter.setVoxelAt(x,y,z,4);
					else
						volIter.setVoxelAt(x,y,z,8);
				}
			}
		}		

		for(uint z = 0; z < OGRE_VOLUME_SIDE_LENGTH; ++z)
		{
			for(uint y = 0; y < OGRE_VOLUME_SIDE_LENGTH; ++y)
			{
				for(uint x = 0; x < OGRE_VOLUME_SIDE_LENGTH; ++x)
				{
					if(
						(z<62)||
						(z>193)||
						(y<78)||
						(y>177)||
						(x<30)||
						(x>225)
						)
					{
						volIter.setVoxelAt(x,y,z,2);
					}
				}
			}
		}		

		//Rooms
		Vector3 centre(128,128,128);
		Vector3 v3dSize(192,96,128);

		uint uHalfX = static_cast<uint>(v3dSize.x / 2);
		uint uHalfY = static_cast<uint>(v3dSize.y / 2);
		uint uHalfZ = static_cast<uint>(v3dSize.z / 2);

		for(uint z = static_cast<uint>(centre.z) - uHalfZ; z < static_cast<uint>(centre.z) + uHalfZ; z++)
		{
			for(uint y = static_cast<uint>(centre.y) - uHalfY; y < static_cast<uint>(centre.y) + uHalfY; y++)
			{
				for(uint x = static_cast<uint>(centre.x) - uHalfX; x < static_cast<uint>(centre.x) + uHalfX; x++)
				{
					volIter.setVoxelAt(x,y,z,0);
				}
			}
		}

		for(uint z = 0; z < OGRE_VOLUME_SIDE_LENGTH; ++z)
		{
			for(uint y = 0; y < OGRE_VOLUME_SIDE_LENGTH; ++y)
			{
				for(uint x = 0; x < OGRE_VOLUME_SIDE_LENGTH; ++x)
				{
					if(
						(x%64 < 8) &&
						(y < 128) &&
						(z>=62)&&
						(z<=193)&&
						(y>=78)&&
						(y<=177)&&
						(x>=30)&&
						(x<=225)
						)
					{
						volIter.setVoxelAt(x,y,z,1);
					}
				}
			}
		}
	}

	void PolyVoxSceneManager::generateMeshDataForRegion(const uint regionX, const uint regionY, const uint regionZ, IndexedSurfacePatch* singleMaterialPatch, IndexedSurfacePatch* multiMaterialPatch) const
	{	
		//IndexedSurfacePatch* surfacePatchResult = new IndexedSurfacePatch;

		//LogManager::getSingleton().logMessage("Generating Mesh Data");
		//First and last voxels in the region
		const uint firstX = regionX * OGRE_REGION_SIDE_LENGTH;
		const uint firstY = regionY * OGRE_REGION_SIDE_LENGTH;
		const uint firstZ = regionZ * OGRE_REGION_SIDE_LENGTH;
		const uint lastX = (std::min)(firstX + OGRE_REGION_SIDE_LENGTH-1,static_cast<uint>(OGRE_VOLUME_SIDE_LENGTH-2));
		const uint lastY = (std::min)(firstY + OGRE_REGION_SIDE_LENGTH-1,static_cast<uint>(OGRE_VOLUME_SIDE_LENGTH-2));
		const uint lastZ = (std::min)(firstZ + OGRE_REGION_SIDE_LENGTH-1,static_cast<uint>(OGRE_VOLUME_SIDE_LENGTH-2));

		//Offset from lower block corner
		const UIntVector3 offset(firstX*2,firstY*2,firstZ*2);

		UIntVector3 vertlist[12];
		uchar vertMaterials[12];
		VolumeIterator volIter(*volumeData);
		volIter.setValidRegion(firstX,firstY,firstZ,lastX,lastY,lastZ);

		//////////////////////////////////////////////////////////////////////////
		//Get mesh data
		//////////////////////////////////////////////////////////////////////////

		//Iterate over each cell in the region
		for(volIter.setPosition(firstX,firstY,firstZ);volIter.isValidForRegion();volIter.moveForwardInRegion())
		{		
			//Current position
			const uint x = volIter.getPosX();
			const uint y = volIter.getPosY();
			const uint z = volIter.getPosZ();

			//LogManager::getSingleton().logMessage("x = " + StringConverter::toString(int(x)) + " y = " + StringConverter::toString(int(y)) + " z = " + StringConverter::toString(int(z)));

			//Voxels values
			const uchar v000 = volIter.getVoxel();
			const uchar v100 = volIter.peekVoxel1px0py0pz();
			const uchar v010 = volIter.peekVoxel0px1py0pz();
			const uchar v110 = volIter.peekVoxel1px1py0pz();
			const uchar v001 = volIter.peekVoxel0px0py1pz();
			const uchar v101 = volIter.peekVoxel1px0py1pz();
			const uchar v011 = volIter.peekVoxel0px1py1pz();
			const uchar v111 = volIter.peekVoxel1px1py1pz();

			//Determine the index into the edge table which tells us which vertices are inside of the surface
			uchar iCubeIndex = 0;

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
				vertlist[0].x = 2*x + 1;
				vertlist[0].y = 2*y;
				vertlist[0].z = 2*z;
				vertMaterials[0] = v000 | v100; //Because one of these is 0, the or operation takes the max.
			}
			if (edgeTable[iCubeIndex] & 2)
			{
				vertlist[1].x = 2*x + 2;
				vertlist[1].y = 2*y + 1;
				vertlist[1].z = 2*z;
				vertMaterials[1] = v100 | v110;
			}
			if (edgeTable[iCubeIndex] & 4)
			{
				vertlist[2].x = 2*x + 1;
				vertlist[2].y = 2*y + 2;
				vertlist[2].z = 2*z;
				vertMaterials[2] = v010 | v110;
			}
			if (edgeTable[iCubeIndex] & 8)
			{
				vertlist[3].x = 2*x;
				vertlist[3].y = 2*y + 1;
				vertlist[3].z = 2*z;
				vertMaterials[3] = v000 | v010;
			}
			if (edgeTable[iCubeIndex] & 16)
			{
				vertlist[4].x = 2*x + 1;
				vertlist[4].y = 2*y;
				vertlist[4].z = 2*z + 2;
				vertMaterials[4] = v001 | v101;
			}
			if (edgeTable[iCubeIndex] & 32)
			{
				vertlist[5].x = 2*x + 2;
				vertlist[5].y = 2*y + 1;
				vertlist[5].z = 2*z + 2;
				vertMaterials[5] = v101 | v111;
			}
			if (edgeTable[iCubeIndex] & 64)
			{
				vertlist[6].x = 2*x + 1;
				vertlist[6].y = 2*y + 2;
				vertlist[6].z = 2*z + 2;
				vertMaterials[6] = v011 | v111;
			}
			if (edgeTable[iCubeIndex] & 128)
			{
				vertlist[7].x = 2*x;
				vertlist[7].y = 2*y + 1;
				vertlist[7].z = 2*z + 2;
				vertMaterials[7] = v001 | v011;
			}
			if (edgeTable[iCubeIndex] & 256)
			{
				vertlist[8].x = 2*x;
				vertlist[8].y = 2*y;
				vertlist[8].z = 2*z + 1;
				vertMaterials[8] = v000 | v001;
			}
			if (edgeTable[iCubeIndex] & 512)
			{
				vertlist[9].x = 2*x + 2;
				vertlist[9].y = 2*y;
				vertlist[9].z = 2*z + 1;
				vertMaterials[9] = v100 | v101;
			}
			if (edgeTable[iCubeIndex] & 1024)
			{
				vertlist[10].x = 2*x + 2;
				vertlist[10].y = 2*y + 2;
				vertlist[10].z = 2*z + 1;
				vertMaterials[10] = v110 | v111;
			}
			if (edgeTable[iCubeIndex] & 2048)
			{
				vertlist[11].x = 2*x;
				vertlist[11].y = 2*y + 2;
				vertlist[11].z = 2*z + 1;
				vertMaterials[11] = v010 | v011;
			}

			for (int i=0;triTable[iCubeIndex][i]!=-1;i+=3)
			{
				//The three vertices forming a triangle
				const UIntVector3 vertex0 = vertlist[triTable[iCubeIndex][i  ]] - offset;
				const UIntVector3 vertex1 = vertlist[triTable[iCubeIndex][i+1]] - offset;
				const UIntVector3 vertex2 = vertlist[triTable[iCubeIndex][i+2]] - offset;

				const uchar material0 = vertMaterials[triTable[iCubeIndex][i  ]];
				const uchar material1 = vertMaterials[triTable[iCubeIndex][i+1]];
				const uchar material2 = vertMaterials[triTable[iCubeIndex][i+2]];


				//If all the materials are the same, we just need one triangle for that material with all the alphas set high.
				if((material0 == material1) && (material1 == material2))
				{
					SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
					SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
					SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
					singleMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				}
				else if(material0 == material1)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material0 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else if(material1 == material2)
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				else
				{
					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material0 + 0.1,1.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material0 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material0 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material1 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material1 + 0.1,1.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material1 + 0.1,0.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}

					{
						SurfaceVertex surfaceVertex0Alpha1(vertex0,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex1Alpha1(vertex1,material2 + 0.1,0.0);
						SurfaceVertex surfaceVertex2Alpha1(vertex2,material2 + 0.1,1.0);
						multiMaterialPatch->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
					}
				}
				//If there not all the same, we need one triangle for each unique material.
				//We'll also need some vertices with low alphas for blending.
				/*else 
				{
				SurfaceVertex surfaceVertex0Alpha0(vertex0,0.0);
				SurfaceVertex surfaceVertex1Alpha0(vertex1,0.0);
				SurfaceVertex surfaceVertex2Alpha0(vertex2,0.0);

				if(material0 == material1)
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				surfacePatchMapResult[material2]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				}
				else if(material1 == material2)
				{
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha1);
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha0);
				}
				else if(material2 == material0)
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				}
				else
				{
				surfacePatchMapResult[material0]->addTriangle(surfaceVertex0Alpha1, surfaceVertex1Alpha0, surfaceVertex2Alpha0);
				surfacePatchMapResult[material1]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha1, surfaceVertex2Alpha0);
				surfacePatchMapResult[material2]->addTriangle(surfaceVertex0Alpha0, surfaceVertex1Alpha0, surfaceVertex2Alpha1);
				}
				}*/
			}//For each triangle
		}//For each cell

		//FIXME - can it happen that we have no vertices or triangles? Should exit early?


		//for(std::map<uchar, IndexedSurfacePatch*>::iterator iterPatch = surfacePatchMapResult.begin(); iterPatch != surfacePatchMapResult.end(); ++iterPatch)
		{

			std::vector<SurfaceVertex>::iterator iterSurfaceVertex = singleMaterialPatch->m_vecVertices.begin();
			while(iterSurfaceVertex != singleMaterialPatch->m_vecVertices.end())
			{
				Vector3 tempNormal = computeNormal((iterSurfaceVertex->getPosition() + offset).toOgreVector3()/2.0f, CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			iterSurfaceVertex = multiMaterialPatch->m_vecVertices.begin();
			while(iterSurfaceVertex != multiMaterialPatch->m_vecVertices.end())
			{
				Vector3 tempNormal = computeNormal((iterSurfaceVertex->getPosition() + offset).toOgreVector3()/2.0f, CENTRAL_DIFFERENCE);
				const_cast<SurfaceVertex&>(*iterSurfaceVertex).setNormal(tempNormal);
				++iterSurfaceVertex;
			}

			uint noOfRemovedVertices = 0;
			//do
			{
				//noOfRemovedVertices = iterPatch->second.decimate();
			}
			//while(noOfRemovedVertices > 10); //We don't worry about the last few vertices - it's not worth the overhead of calling the function.
		}

		//LogManager::getSingleton().logMessage("Finished Generating Mesh Data");

		//return singleMaterialPatch;
	}

	Vector3 PolyVoxSceneManager::computeNormal(const Vector3& position, NormalGenerationMethod normalGenerationMethod) const
	{
		VolumeIterator volIter(*volumeData); //FIXME - save this somewhere - could be expensive to create?

		//LogManager::getSingleton().logMessage("In Loop");
		const float posX = position.x;
		const float posY = position.y;
		const float posZ = position.z;

		const uint floorX = static_cast<uint>(posX);
		const uint floorY = static_cast<uint>(posY);
		const uint floorZ = static_cast<uint>(posZ);

		Vector3 result;


		if(normalGenerationMethod == SOBEL)
		{
			volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY),static_cast<uint>(posZ));
			const Vector3 gradFloor = volIter.getSobelGradient();
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint>(posX+1.0),static_cast<uint>(posY),static_cast<uint>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY+1.0),static_cast<uint>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY),static_cast<uint>(posZ+1.0));					
			}
			const Vector3 gradCeil = volIter.getSobelGradient();
			result = ((gradFloor + gradCeil) * -1.0);
			if(result.squaredLength() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == CENTRAL_DIFFERENCE)
		{
			volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY),static_cast<uint>(posZ));
			const Vector3 gradFloor = volIter.getCentralDifferenceGradient();
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint>(posX+1.0),static_cast<uint>(posY),static_cast<uint>(posZ));
			}
			if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY+1.0),static_cast<uint>(posZ));
			}
			if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{			
				volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY),static_cast<uint>(posZ+1.0));					
			}
			const Vector3 gradCeil = volIter.getCentralDifferenceGradient();
			result = ((gradFloor + gradCeil) * -1.0);
			if(result.squaredLength() < 0.0001)
			{
				//Operation failed - fall back on simple gradient estimation
				normalGenerationMethod = SIMPLE;
			}
		}
		if(normalGenerationMethod == SIMPLE)
		{
			volIter.setPosition(static_cast<uint>(posX),static_cast<uint>(posY),static_cast<uint>(posZ));
			const uchar uFloor = volIter.getVoxel() > 0 ? 1 : 0;
			if((posX - floorX) > 0.25) //The result should be 0.0 or 0.5
			{					
				uchar uCeil = volIter.peekVoxel1px0py0pz() > 0 ? 1 : 0;
				result = Vector3(uFloor - uCeil,0.0,0.0);
			}
			else if((posY - floorY) > 0.25) //The result should be 0.0 or 0.5
			{
				uchar uCeil = volIter.peekVoxel0px1py0pz() > 0 ? 1 : 0;
				result = Vector3(0.0,uFloor - uCeil,0.0);
			}
			else if((posZ - floorZ) > 0.25) //The result should be 0.0 or 0.5
			{
				uchar uCeil = volIter.peekVoxel0px0py1pz() > 0 ? 1 : 0;
				result = Vector3(0.0, 0.0,uFloor - uCeil);					
			}
		}
		return result;
	}

	void PolyVoxSceneManager::markVoxelChanged(uint x, uint y, uint z)
	{
		//If we are not on a boundary, just mark one region.
		if((x % OGRE_REGION_SIDE_LENGTH != 0) &&
			(x % OGRE_REGION_SIDE_LENGTH != OGRE_REGION_SIDE_LENGTH-1) &&
			(y % OGRE_REGION_SIDE_LENGTH != 0) &&
			(y % OGRE_REGION_SIDE_LENGTH != OGRE_REGION_SIDE_LENGTH-1) &&
			(z % OGRE_REGION_SIDE_LENGTH != 0) &&
			(z % OGRE_REGION_SIDE_LENGTH != OGRE_REGION_SIDE_LENGTH-1))
		{
			surfaceUpToDate[x >> OGRE_REGION_SIDE_LENGTH_POWER][y >> OGRE_REGION_SIDE_LENGTH_POWER][z >> OGRE_REGION_SIDE_LENGTH_POWER] = false;
		}
		else //Mark surrounding block as well
		{
			const uint regionX = x >> OGRE_REGION_SIDE_LENGTH_POWER;
			const uint regionY = y >> OGRE_REGION_SIDE_LENGTH_POWER;
			const uint regionZ = z >> OGRE_REGION_SIDE_LENGTH_POWER;

			const uint minRegionX = (std::max)(uint(0),regionX-1);
			const uint minRegionY = (std::max)(uint(0),regionY-1);
			const uint minRegionZ = (std::max)(uint(0),regionZ-1);

			const uint maxRegionX = (std::min)(uint(OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS-1),regionX+1);
			const uint maxRegionY = (std::min)(uint(OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS-1),regionY+1);
			const uint maxRegionZ = (std::min)(uint(OGRE_VOLUME_SIDE_LENGTH_IN_REGIONS-1),regionZ+1);

			for(uint zCt = minRegionZ; zCt <= maxRegionZ; zCt++)
			{
				for(uint yCt = minRegionY; yCt <= maxRegionY; yCt++)
				{
					for(uint xCt = minRegionX; xCt <= maxRegionX; xCt++)
					{
						surfaceUpToDate[xCt][yCt][zCt] = false;
					}
				}
			}
		}
	}

	void PolyVoxSceneManager::markRegionChanged(uint firstX, uint firstY, uint firstZ, uint lastX, uint lastY, uint lastZ)
	{
		const uint firstRegionX = firstX >> OGRE_REGION_SIDE_LENGTH_POWER;
		const uint firstRegionY = firstY >> OGRE_REGION_SIDE_LENGTH_POWER;
		const uint firstRegionZ = firstZ >> OGRE_REGION_SIDE_LENGTH_POWER;

		const uint lastRegionX = lastX >> OGRE_REGION_SIDE_LENGTH_POWER;
		const uint lastRegionY = lastY >> OGRE_REGION_SIDE_LENGTH_POWER;
		const uint lastRegionZ = lastZ >> OGRE_REGION_SIDE_LENGTH_POWER;

		for(uint zCt = firstRegionZ; zCt <= lastRegionZ; zCt++)
		{
			for(uint yCt = firstRegionY; yCt <= lastRegionY; yCt++)
			{
				for(uint xCt = firstRegionX; xCt <= lastRegionX; xCt++)
				{
					surfaceUpToDate[xCt][yCt][zCt] = false;
				}
			}
		}
	}

	void PolyVoxSceneManager::doRegionGrowing(uint xStart, uint yStart, uint zStart, uchar value)
	{
		volumeData->regionGrow(xStart,yStart,zStart,value);
		//FIXME - keep track of what has changed...
		markRegionChanged(0,0,0,OGRE_VOLUME_SIDE_LENGTH-1,OGRE_VOLUME_SIDE_LENGTH-1,OGRE_VOLUME_SIDE_LENGTH-1);
	}

	bool PolyVoxSceneManager::saveScene(const String& filename)
	{
		volumeData->saveToFile(filename);
		return true; //FIXME - check for error...
	}

	uint PolyVoxSceneManager::getSideLength(void)
	{
		return OGRE_VOLUME_SIDE_LENGTH;
	}

	uchar PolyVoxSceneManager::getMaterialIndexAt(uint uX, uint uY, uint uZ)
	{
		if(volumeData->containsPoint(IntVector3(uX,uY,uZ),0))
		{
			VolumeIterator volIter(*volumeData);
			return volIter.getVoxelAt(uX,uY,uZ);
		}
		else
		{
			return 0;
		}
	}

	void PolyVoxSceneManager::setNormalGenerationMethod(NormalGenerationMethod method)
	{
		m_normalGenerationMethod = method;
	}

	bool PolyVoxSceneManager::containsPoint(Vector3 pos, float boundary)
	{
		return volumeData->containsPoint(pos, boundary);
	}

	bool PolyVoxSceneManager::containsPoint(IntVector3 pos, uint boundary)
	{
		return volumeData->containsPoint(pos, boundary);
	}

	void PolyVoxSceneManager::createAxis(uint uSideLength)
	{
		float fSideLength = static_cast<float>(uSideLength);
		float fHalfSideLength = fSideLength/2.0;
		Vector3 vecOriginAndConeScale(4.0,4.0,4.0);

		//Create the main node for the axes
		m_axisNode = getRootSceneNode()->createChildSceneNode();

		//Create sphrere representing origin
		SceneNode* originNode = m_axisNode->createChildSceneNode();
		Entity *originSphereEntity = createEntity( "Origin Sphere", "Sphere.mesh" );
		originSphereEntity->setMaterialName("WhiteMaterial");
		originNode->attachObject(originSphereEntity);
		originNode->scale(vecOriginAndConeScale);

		//Create arrow body for x-axis
		SceneNode *xAxisCylinderNode = m_axisNode->createChildSceneNode();
		Entity *xAxisCylinderEntity = createEntity( "X Axis Cylinder", "Cylinder.mesh" );
		xAxisCylinderEntity->setMaterialName("RedMaterial");
		xAxisCylinderNode->attachObject(xAxisCylinderEntity);			
		xAxisCylinderNode->scale(Vector3(1.0,1.0,fHalfSideLength-4.0));
		xAxisCylinderNode->translate(Vector3(fHalfSideLength,0.0,0.0));
		xAxisCylinderNode->rotate(Vector3::UNIT_Y, Radian(1.5707));

		//Create arrow head for x-axis
		SceneNode *xAxisConeNode = m_axisNode->createChildSceneNode();
		Entity *xAxisConeEntity = createEntity( "X Axis Cone", "Cone.mesh" );
		xAxisConeEntity->setMaterialName("RedMaterial");
		xAxisConeNode->attachObject(xAxisConeEntity);		
		xAxisConeNode->rotate(Vector3::UNIT_Y, Radian(1.5707));
		xAxisConeNode->scale(vecOriginAndConeScale);
		xAxisConeNode->translate(Vector3(fSideLength-4.0,0.0,0.0));

		//Create arrow body for y-axis
		SceneNode *yAxisCylinderNode = m_axisNode->createChildSceneNode();
		Entity *yAxisCylinderEntity = createEntity( "Y Axis Cylinder", "Cylinder.mesh" );
		yAxisCylinderEntity->setMaterialName("GreenMaterial");
		yAxisCylinderNode->attachObject(yAxisCylinderEntity);		
		yAxisCylinderNode->scale(Vector3(1.0,1.0,fHalfSideLength-4.0));
		yAxisCylinderNode->translate(Vector3(0.0,fHalfSideLength,0.0));
		yAxisCylinderNode->rotate(Vector3::UNIT_X, Radian(1.5707));

		//Create arrow head for y-axis
		SceneNode *yAxisConeNode = m_axisNode->createChildSceneNode();
		Entity *yAxisConeEntity = createEntity( "Y Axis Cone", "Cone.mesh" );
		yAxisConeEntity->setMaterialName("GreenMaterial");
		yAxisConeNode->attachObject(yAxisConeEntity);		
		yAxisConeNode->rotate(Vector3::UNIT_X, Radian(-1.5707));
		yAxisConeNode->scale(vecOriginAndConeScale);
		yAxisConeNode->translate(Vector3(0.0,fSideLength-4.0,0.0));

		//Create arrow body for z-axis
		SceneNode *zAxisCylinderNode = m_axisNode->createChildSceneNode();
		Entity *zAxisCylinderEntity = createEntity( "Z Axis Cylinder", "Cylinder.mesh" );
		zAxisCylinderEntity->setMaterialName("BlueMaterial");
		zAxisCylinderNode->attachObject(zAxisCylinderEntity);
		zAxisCylinderNode->translate(Vector3(0.0,0.0,fHalfSideLength));
		zAxisCylinderNode->scale(Vector3(1.0,1.0,fHalfSideLength-4.0));	

		//Create arrow head for z-axis
		SceneNode *zAxisConeNode = m_axisNode->createChildSceneNode();
		Entity *zAxisConeEntity = createEntity( "Z Axis Cone", "Cone.mesh" );
		zAxisConeEntity->setMaterialName("BlueMaterial");
		zAxisConeNode->attachObject(zAxisConeEntity);
		zAxisConeNode->translate(Vector3(0.0,0.0,fSideLength-4.0));
		zAxisConeNode->scale(vecOriginAndConeScale);

		//Create remainder of box		
		ManualObject* remainingBox = createManualObject("Remaining Box");
		remainingBox->begin("BaseWhiteNoLighting",RenderOperation::OT_LINE_LIST);
		remainingBox->position(0.0,			0.0,			0.0			);	remainingBox->position(0.0,			0.0,			fSideLength	);
		remainingBox->position(0.0,			fSideLength,	0.0			);	remainingBox->position(0.0,			fSideLength,	fSideLength	);
		remainingBox->position(fSideLength, 0.0,			0.0			);	remainingBox->position(fSideLength, 0.0,			fSideLength	);
		remainingBox->position(fSideLength, fSideLength,	0.0			);	remainingBox->position(fSideLength, fSideLength,	fSideLength	);

		remainingBox->position(0.0,			0.0,			0.0			);	remainingBox->position(0.0,			fSideLength,	0.0			);
		remainingBox->position(0.0,			0.0,			fSideLength	);	remainingBox->position(0.0,			fSideLength,	fSideLength	);
		remainingBox->position(fSideLength, 0.0,			0.0			);	remainingBox->position(fSideLength, fSideLength,	0.0			);
		remainingBox->position(fSideLength, 0.0,			fSideLength	);	remainingBox->position(fSideLength, fSideLength,	fSideLength	);

		remainingBox->position(0.0,			0.0,			0.0			);	remainingBox->position(fSideLength, 0.0,			0.0			);
		remainingBox->position(0.0,			0.0,			fSideLength	);	remainingBox->position(fSideLength, 0.0,			fSideLength	);
		remainingBox->position(0.0,			fSideLength,	0.0			);	remainingBox->position(fSideLength, fSideLength,	0.0			);
		remainingBox->position(0.0,			fSideLength,	fSideLength	);	remainingBox->position(fSideLength, fSideLength,	fSideLength	);
		remainingBox->end();
		SceneNode *remainingBoxNode = m_axisNode->createChildSceneNode();
		remainingBoxNode->attachObject(remainingBox);
	}

	void PolyVoxSceneManager::setAxisVisible(bool visible)
	{
		if(m_axisNode)
			m_axisNode->setVisible(visible);
	}
}
