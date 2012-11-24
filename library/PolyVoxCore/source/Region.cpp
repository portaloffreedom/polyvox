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

#include "PolyVoxCore/Region.h"

#include <limits>

namespace PolyVox
{
	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	const Region Region::MaxRegion
	(
		Vector3DInt32((std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::min)()),
		Vector3DInt32((std::numeric_limits<int32_t>::max)(), (std::numeric_limits<int32_t>::max)(), (std::numeric_limits<int32_t>::max)())
	);

	////////////////////////////////////////////////////////////////////////////////
	/// This Region is not considered valid as defined by isValid(). It's main application
	/// is to initialise a Region to this value and then() accumulate positions. The result
	/// of this will be a Region which encompasses all positions specified.
	////////////////////////////////////////////////////////////////////////////////
	const Region Region::InvertedRegion
	(
		Vector3DInt32((std::numeric_limits<int32_t>::max)(), (std::numeric_limits<int32_t>::max)(), (std::numeric_limits<int32_t>::max)()),
		Vector3DInt32((std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::min)(), (std::numeric_limits<int32_t>::min)())
	);

	////////////////////////////////////////////////////////////////////////////////
	/// Constructs a Region and clears all extents to zero.
	////////////////////////////////////////////////////////////////////////////////
	Region::Region()
		:m_iLowerX(0)
		,m_iLowerY(0)
		,m_iLowerZ(0)
		,m_iUpperX(0)
		,m_iUpperY(0)
		,m_iUpperZ(0)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Constructs a Region and sets the lower and upper corners to the specified values.
	/// \param v3dLowerCorner The desired lower corner of the Region.
	/// \param v3dUpperCorner The desired upper corner of the Region.
	////////////////////////////////////////////////////////////////////////////////
	Region::Region(const Vector3DInt32& v3dLowerCorner, const Vector3DInt32& v3dUpperCorner)
		:m_iLowerX(v3dLowerCorner.getX())
		,m_iLowerY(v3dLowerCorner.getY())
		,m_iLowerZ(v3dLowerCorner.getZ())
		,m_iUpperX(v3dUpperCorner.getX())
		,m_iUpperY(v3dUpperCorner.getY())
		,m_iUpperZ(v3dUpperCorner.getZ())
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Constructs a Region and sets the extents to the specified values.
	/// \param iLowerX The desired lower 'x' extent of the Region.
	/// \param iLowerY The desired lower 'y' extent of the Region.
	/// \param iLowerZ The desired lower 'z' extent of the Region.
	/// \param iUpperX The desired upper 'x' extent of the Region.
	/// \param iUpperY The desired upper 'y' extent of the Region.
	/// \param iUpperZ The desired upper 'z' extent of the Region.
	////////////////////////////////////////////////////////////////////////////////
	Region::Region(int32_t iLowerX, int32_t iLowerY, int32_t iLowerZ, int32_t iUpperX, int32_t iUpperY, int32_t iUpperZ)
		:m_iLowerX(iLowerX)
		,m_iLowerY(iLowerY)
		,m_iLowerZ(iLowerZ)
		,m_iUpperX(iUpperX)
		,m_iUpperY(iUpperY)
		,m_iUpperZ(iUpperZ)
	{
	}

	////////////////////////////////////////////////////////////////////////////////
	/// Two regions are considered equal if all their extents match.
	/// \param rhs The Region to compare to.
    /// \return true if the Regions match.
    /// \sa operator!=
	////////////////////////////////////////////////////////////////////////////////
    bool Region::operator==(const Region& rhs) const
    {
		return ((m_iLowerX == rhs.m_iLowerX) && (m_iLowerY == rhs.m_iLowerY) && (m_iLowerZ == rhs.m_iLowerZ)
			&&  (m_iUpperX == rhs.m_iUpperX) && (m_iUpperY == rhs.m_iUpperY) && (m_iUpperZ == rhs.m_iUpperZ));
    }

	////////////////////////////////////////////////////////////////////////////////
	/// Two regions are considered different if any of their extents differ.
	/// \param rhs The Region to compare to.
    /// \return true if the Regions are different.
    /// \sa operator==
	////////////////////////////////////////////////////////////////////////////////
    bool Region::operator!=(const Region& rhs) const
    {
		return !(*this == rhs);
    }

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in all directions. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPoint(const Vector3DFloat& pos, float boundary) const
	{
		return (pos.getX() <= m_iUpperX - boundary)
			&& (pos.getY() <= m_iUpperY - boundary)
			&& (pos.getZ() <= m_iUpperZ - boundary)
			&& (pos.getX() >= m_iLowerX + boundary)
			&& (pos.getY() >= m_iLowerY + boundary)
			&& (pos.getZ() >= m_iLowerZ + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in all directions. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPoint(const Vector3DInt32& pos, uint8_t boundary) const
	{
		return (pos.getX() <= m_iUpperX - boundary)
			&& (pos.getY() <= m_iUpperY - boundary) 
			&& (pos.getZ() <= m_iUpperZ - boundary)
			&& (pos.getX() >= m_iLowerX + boundary)
			&& (pos.getY() >= m_iLowerY + boundary)
			&& (pos.getZ() >= m_iLowerZ + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in the 'x' direction. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPointInX(float pos, float boundary) const
	{
		return (pos <= m_iUpperX - boundary)
			&& (pos >= m_iLowerX + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in the 'x' direction. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPointInX(int32_t pos, uint8_t boundary) const
	{
		return (pos <= m_iUpperX - boundary)
			&& (pos >= m_iLowerX + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in the 'y' direction. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPointInY(float pos, float boundary) const
	{
		return (pos <= m_iUpperY - boundary)
			&& (pos >= m_iLowerY + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in the 'y' direction. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPointInY(int32_t pos, uint8_t boundary) const
	{
		return (pos <= m_iUpperY - boundary) 
			&& (pos >= m_iLowerY + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in the 'z' direction. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPointInZ(float pos, float boundary) const
	{
		return (pos <= m_iUpperZ - boundary)
			&& (pos >= m_iLowerZ + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The boundary value can be used to ensure a position is only considered to be inside
	/// the Region if it is that far in in the 'z' direction. Also, the test is inclusive such 
	/// that positions lying exactly on the edge of the Region are considered to be inside it.
	/// \param pos The position to test.
	/// \param boundary The desired boundary value.
	////////////////////////////////////////////////////////////////////////////////
	bool Region::containsPointInZ(int32_t pos, uint8_t boundary) const
	{
		return (pos <= m_iUpperZ - boundary)
			&& (pos >= m_iLowerZ + boundary);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// After calling this functions, the extents of this Region are given by the union
	/// of this Region and the one it was cropped to.
	/// \param other The Region to crop to.
	////////////////////////////////////////////////////////////////////////////////
	void Region::cropTo(const Region& other)
	{
		m_iLowerX = ((std::max)(m_iLowerX, other.m_iLowerX));
		m_iLowerY = ((std::max)(m_iLowerY, other.m_iLowerY));
		m_iLowerZ = ((std::max)(m_iLowerZ, other.m_iLowerZ));
		m_iUpperX = ((std::min)(m_iUpperX, other.m_iUpperX));
		m_iUpperY = ((std::min)(m_iUpperY, other.m_iUpperY));
		m_iUpperZ = ((std::min)(m_iUpperZ, other.m_iUpperZ));
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The same amount of dilation is applied in all directions. Negative dilations
	/// are possible but you should prefer the erode() function for clarity.
	/// \param iAmount The amount to dilate by.
	////////////////////////////////////////////////////////////////////////////////
	void Region::dilate(int32_t iAmount)
	{
		m_iLowerX -= iAmount;
		m_iLowerY -= iAmount;
		m_iLowerZ -= iAmount;

		m_iUpperX += iAmount;
		m_iUpperY += iAmount;
		m_iUpperZ += iAmount;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The dilation can be specified seperatly for each direction. Negative dilations
	/// are possible but you should prefer the erode() function for clarity.
	/// \param iAmountX The amount to dilate by in 'x'.
	/// \param iAmountY The amount to dilate by in 'y'.
	/// \param iAmountZ The amount to dilate by in 'z'.
	////////////////////////////////////////////////////////////////////////////////
	void Region::dilate(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ)
	{
		m_iLowerX -= iAmountX;
		m_iLowerY -= iAmountY;
		m_iLowerZ -= iAmountZ;

		m_iUpperX += iAmountX;
		m_iUpperY += iAmountY;
		m_iUpperZ += iAmountZ;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The dilation can be specified seperatly for each direction. Negative dilations
	/// are possible but you should prefer the erode() function for clarity.
	/// \param v3dAmount The amount to dilate by (one components for each direction).
	////////////////////////////////////////////////////////////////////////////////
	void Region::dilate(const Vector3DInt32& v3dAmount)
	{
		dilate(v3dAmount.getX(), v3dAmount.getY(), v3dAmount.getZ());
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The same amount of erosion is applied in all directions. Negative erosions
	/// are possible but you should prefer the dilate() function for clarity.
	/// \param iAmount The amount to erode by.
	////////////////////////////////////////////////////////////////////////////////
	void Region::erode(int32_t iAmount)
	{
		m_iLowerX += iAmount;
		m_iLowerY += iAmount;
		m_iLowerZ += iAmount;

		m_iUpperX -= iAmount;
		m_iUpperY -= iAmount;
		m_iUpperZ -= iAmount;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The erosion can be specified seperatly for each direction. Negative erosions
	/// are possible but you should prefer the dilate() function for clarity.
	/// \param iAmountX The amount to erode by in 'x'.
	/// \param iAmountY The amount to erode by in 'y'.
	/// \param iAmountZ The amount to erode by in 'z'.
	////////////////////////////////////////////////////////////////////////////////
	void Region::erode(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ)
	{
		m_iLowerX += iAmountX;
		m_iLowerY += iAmountY;
		m_iLowerZ += iAmountZ;

		m_iUpperX -= iAmountX;
		m_iUpperY -= iAmountY;
		m_iUpperZ -= iAmountZ;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// The erosion can be specified seperatly for each direction. Negative erosions
	/// are possible but you should prefer the dilate() function for clarity.
	/// \param v3dAmount The amount to erode by (one components for each direction).
	////////////////////////////////////////////////////////////////////////////////
	void Region::erode(const Vector3DInt32& v3dAmount)
	{
		erode(v3dAmount.getX(), v3dAmount.getY(), v3dAmount.getZ());
	}

	////////////////////////////////////////////////////////////////////////////////
	////////////////////////////////////////////////////////////////////////////////
	bool Region::isValid(void)
	{
		return (m_iUpperX >= m_iLowerX) && (m_iUpperY >= m_iLowerY) && (m_iUpperZ >= m_iLowerZ);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// \param iAmountX The amount to move the Region by in 'x'.
	/// \param iAmountY The amount to move the Region by in 'y'.
	/// \param iAmountZ The amount to move the Region by in 'z'.
	////////////////////////////////////////////////////////////////////////////////
	void Region::shift(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ)
	{
		shiftLowerCorner(iAmountX, iAmountY, iAmountZ);
		shiftUpperCorner(iAmountX, iAmountY, iAmountZ);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// \param v3dAmount The amount to move the Region by.
	////////////////////////////////////////////////////////////////////////////////
	void Region::shift(const Vector3DInt32& v3dAmount)
	{
		shiftLowerCorner(v3dAmount);
		shiftUpperCorner(v3dAmount);
	}

	////////////////////////////////////////////////////////////////////////////////
	/// \param iAmountX The amount to move the lower corner by in 'x'.
	/// \param iAmountY The amount to move the lower corner by in 'y'.
	/// \param iAmountZ The amount to move the lower corner by in 'z'.
	////////////////////////////////////////////////////////////////////////////////
	void Region::shiftLowerCorner(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ)
	{
		m_iLowerX += iAmountX;
		m_iLowerY += iAmountY;
		m_iLowerZ += iAmountZ;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// \param v3dAmount The amount to move the lower corner by.
	////////////////////////////////////////////////////////////////////////////////
	void Region::shiftLowerCorner(const Vector3DInt32& v3dAmount)
	{
		shiftLowerCorner(v3dAmount.getX(), v3dAmount.getY(), v3dAmount.getZ());
	}

	////////////////////////////////////////////////////////////////////////////////
	/// \param iAmountX The amount to move the upper corner by in 'x'.
	/// \param iAmountY The amount to move the upper corner by in 'y'.
	/// \param iAmountZ The amount to move the upper corner by in 'z'.
	////////////////////////////////////////////////////////////////////////////////
	void Region::shiftUpperCorner(int32_t iAmountX, int32_t iAmountY, int32_t iAmountZ)
	{
		m_iUpperX += iAmountX;
		m_iUpperY += iAmountY;
		m_iUpperZ += iAmountZ;
	}

	////////////////////////////////////////////////////////////////////////////////
	/// \param v3dAmount The amount to move the upper corner by.
	////////////////////////////////////////////////////////////////////////////////
	void Region::shiftUpperCorner(const Vector3DInt32& v3dAmount)
	{
		shiftUpperCorner(v3dAmount.getX(), v3dAmount.getY(), v3dAmount.getZ());
	}
}
