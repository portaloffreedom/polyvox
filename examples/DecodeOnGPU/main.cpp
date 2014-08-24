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

#include "OpenGLWidget.h"

#include "PolyVoxCore/CubicSurfaceExtractor.h"
#include "PolyVoxCore/MarchingCubesSurfaceExtractor.h"
#include "PolyVoxCore/Mesh.h"
#include "PolyVoxCore/SimpleVolume.h"

#include <QApplication>

//Use the PolyVox namespace
using namespace PolyVox;

void createSphereInVolume(SimpleVolume<uint8_t>& volData, float fRadius)
{
	//This vector hold the position of the center of the volume
	Vector3DFloat v3dVolCenter(volData.getWidth() / 2, volData.getHeight() / 2, volData.getDepth() / 2);

	//This three-level for loop iterates over every voxel in the volume
	for (int z = 0; z < volData.getDepth(); z++)
	{
		for (int y = 0; y < volData.getHeight(); y++)
		{
			for (int x = 0; x < volData.getWidth(); x++)
			{
				//Store our current position as a vector...
				Vector3DFloat v3dCurrentPos(x,y,z);	
				//And compute how far the current position is from the center of the volume
				float fDistToCenter = (v3dCurrentPos - v3dVolCenter).length();

				uint8_t uVoxelValue = 0;

				//If the current voxel is less than 'radius' units from the center then we make it solid.
				if(fDistToCenter <= fRadius)
				{
					//Our new voxel value
					uVoxelValue = 255;
				}

				//Wrte the voxel value into the volume	
				volData.setVoxelAt(x, y, z, uVoxelValue);
			}
		}
	}
}

OpenGLMeshData buildOpenGLMeshData(const PolyVox::Mesh< PolyVox::MarchingCubesVertex< uint8_t > >& surfaceMesh, const PolyVox::Vector3DInt32& translation = PolyVox::Vector3DInt32(0, 0, 0), float scale = 1.0f)
{
	// Convienient access to the vertices and indices
	const auto& vecIndices = surfaceMesh.getIndices();
	const auto& vecVertices = surfaceMesh.getVertices();

	// This struct holds the OpenGL properties (buffer handles, etc) which will be used
	// to render our mesh. We copy the data from the PolyVox mesh into this structure.
	OpenGLMeshData meshData;

	// Create the VAO for the mesh
	glGenVertexArrays(1, &(meshData.vertexArrayObject));
	glBindVertexArray(meshData.vertexArrayObject);

	// The GL_ARRAY_BUFFER will contain the list of vertex positions
	glGenBuffers(1, &(meshData.vertexBuffer));
	glBindBuffer(GL_ARRAY_BUFFER, meshData.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vecVertices.size() * sizeof(MarchingCubesVertex< uint8_t >), vecVertices.data(), GL_STATIC_DRAW);

	// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
	glGenBuffers(1, &(meshData.indexBuffer));
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, meshData.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vecIndices.size() * sizeof(uint32_t), vecIndices.data(), GL_STATIC_DRAW);

	// Every surface extractor outputs valid positions for the vertices, so tell OpenGL how these are laid out
	glEnableVertexAttribArray(0); // Attrib '0' is the vertex positions
	glVertexAttribIPointer(0, 3, GL_UNSIGNED_SHORT, sizeof(MarchingCubesVertex< uint8_t >), (GLvoid*)(offsetof(MarchingCubesVertex< uint8_t >, encodedPosition))); //take the first 3 floats from every sizeof(decltype(vecVertices)::value_type)

	// Some surface extractors also generate normals, so tell OpenGL how these are laid out. If a surface extractor
	// does not generate normals then nonsense values are written into the buffer here and sghould be ignored by the
	// shader. This is mostly just to simplify this example code - in a real application you will know whether your
	// chosen surface extractor generates normals and can skip uploading them if not.
	glEnableVertexAttribArray(1); // Attrib '1' is the vertex normals.
	glVertexAttribIPointer(1, 1, GL_UNSIGNED_SHORT, sizeof(MarchingCubesVertex< uint8_t >), (GLvoid*)(offsetof(MarchingCubesVertex< uint8_t >, encodedNormal)));

	// Finally a surface extractor will probably output additional data. This is highly application dependant. For this example code 
	// we're just uploading it as a set of bytes which we can read individually, but real code will want to do something specialised here.
	glEnableVertexAttribArray(2); //We're talking about shader attribute '2'
	GLint size = (std::min)(sizeof(uint8_t), size_t(4)); // Can't upload more that 4 components (vec4 is GLSL's biggest type)
	glVertexAttribIPointer(2, size, GL_UNSIGNED_BYTE, sizeof(MarchingCubesVertex< uint8_t >), (GLvoid*)(offsetof(MarchingCubesVertex< uint8_t >, data)));

	// We're done uploading and can now unbind.
	glBindVertexArray(0);

	// A few additional properties can be copied across for use during rendering.
	meshData.noOfIndices = vecIndices.size();
	meshData.translation = QVector3D(translation.getX(), translation.getY(), translation.getZ());
	meshData.scale = scale;

	// Set 16 or 32-bit index buffer size.
	meshData.indexType = sizeof(PolyVox::Mesh< PolyVox::MarchingCubesVertex< uint8_t > >::IndexType) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT;

	return meshData;
}

int main(int argc, char *argv[])
{
	//Create and show the Qt OpenGL window
	QApplication app(argc, argv);
	OpenGLWidget openGLWidget(0);
	openGLWidget.show();

	QSharedPointer<QGLShaderProgram> shader(new QGLShaderProgram);

	if (!shader->addShaderFromSourceFile(QGLShader::Vertex, ":/decode.vert"))
	{
		std::cerr << shader->log().toStdString() << std::endl;
		exit(EXIT_FAILURE);
	}

	if (!shader->addShaderFromSourceFile(QGLShader::Fragment, ":/decode.frag"))
	{
		std::cerr << shader->log().toStdString() << std::endl;
		exit(EXIT_FAILURE);
	}

	openGLWidget.setShader(shader);

	//Create an empty volume and then place a sphere in it
	SimpleVolume<uint8_t> volData(PolyVox::Region(Vector3DInt32(0,0,0), Vector3DInt32(63, 63, 63)));
	createSphereInVolume(volData, 30);

	// Extract the surface for the specified region of the volume. Uncomment the line for the kind of surface extraction you want to see.
	//auto mesh = extractCubicMesh(&volData, volData.getEnclosingRegion());
	auto mesh = extractMarchingCubesMesh(&volData, volData.getEnclosingRegion());

	// The surface extractor outputs the mesh in an efficient compressed format which is not directly suitable for rendering. The easiest approach is to 
	// decode this on the CPU as shown below, though more advanced applications can upload the compressed mesh to the GPU and decompress in shader code.
	//auto decodedMesh = decodeMesh(mesh);

	//Pass the surface to the OpenGL window
	OpenGLMeshData meshData = buildOpenGLMeshData(mesh);
	openGLWidget.addMeshData(meshData);

	openGLWidget.setViewableRegion(volData.getEnclosingRegion());

	//Run the message pump.
	return app.exec();
}