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

#include "PolyVoxImpl/Utility.h"
#include "Vector.h"

#include <cassert>
#include <cstring> //For memcpy
#include <limits>
#include <stdexcept> //for std::invalid_argument

namespace PolyVox
{
	template <typename VoxelType>
	RawVolume<VoxelType>::Block::Block(uint16_t uSideLength)
		:m_uSideLength(0)
		,m_uSideLengthPower(0)
		,m_tUncompressedData(0)
	{
		if(uSideLength != 0)
		{
			initialise(uSideLength);
		}
	}

	template <typename VoxelType>
	uint16_t RawVolume<VoxelType>::Block::getSideLength(void) const
	{
		return m_uSideLength;
	}

	template <typename VoxelType>
	VoxelType RawVolume<VoxelType>::Block::getVoxelAt(uint16_t uXPos, uint16_t uYPos, uint16_t uZPos) const
	{
		assert(uXPos < m_uSideLength);
		assert(uYPos < m_uSideLength);
		assert(uZPos < m_uSideLength);

		assert(m_tUncompressedData);

		return m_tUncompressedData
			[
				uXPos + 
				uYPos * m_uSideLength + 
				uZPos * m_uSideLength * m_uSideLength
			];
	}

	template <typename VoxelType>
	VoxelType RawVolume<VoxelType>::Block::getVoxelAt(const Vector3DUint16& v3dPos) const
	{
		return getVoxelAt(v3dPos.getX(), v3dPos.getY(), v3dPos.getZ());
	}

	template <typename VoxelType>
	void RawVolume<VoxelType>::Block::setVoxelAt(uint16_t uXPos, uint16_t uYPos, uint16_t uZPos, VoxelType tValue)
	{
		assert(uXPos < m_uSideLength);
		assert(uYPos < m_uSideLength);
		assert(uZPos < m_uSideLength);

		assert(m_tUncompressedData);

		m_tUncompressedData
		[
			uXPos + 
			uYPos * m_uSideLength + 
			uZPos * m_uSideLength * m_uSideLength
		] = tValue;
	}

	template <typename VoxelType>
	void RawVolume<VoxelType>::Block::setVoxelAt(const Vector3DUint16& v3dPos, VoxelType tValue)
	{
		setVoxelAt(v3dPos.getX(), v3dPos.getY(), v3dPos.getZ(), tValue);
	}

	template <typename VoxelType>
	void RawVolume<VoxelType>::Block::fill(VoxelType tValue)
	{
		const uint32_t uNoOfVoxels = m_uSideLength * m_uSideLength * m_uSideLength;
		std::fill(m_tUncompressedData, m_tUncompressedData + uNoOfVoxels, tValue);
	}

	template <typename VoxelType>
	void RawVolume<VoxelType>::Block::initialise(uint16_t uSideLength)
	{
		//Debug mode validation
		assert(isPowerOf2(uSideLength));

		//Release mode validation
		if(!isPowerOf2(uSideLength))
		{
			throw std::invalid_argument("Block side length must be a power of two.");
		}

		//Compute the side length		
		m_uSideLength = uSideLength;
		m_uSideLengthPower = logBase2(uSideLength);

		m_tUncompressedData = new VoxelType[m_uSideLength * m_uSideLength * m_uSideLength];

		RawVolume<VoxelType>::Block::fill(VoxelType());
	}

	template <typename VoxelType>
	uint32_t RawVolume<VoxelType>::Block::calculateSizeInBytes(void)
	{
		uint32_t uSizeInBytes = sizeof(Block);
		uSizeInBytes += sizeof(VoxelType) * m_uSideLength * m_uSideLength * m_uSideLength;
		return  uSizeInBytes;
	}
}
