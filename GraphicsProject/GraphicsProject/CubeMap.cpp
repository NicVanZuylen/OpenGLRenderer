#include "CubeMap.h"
#include "GLAD/glad.h"
#include <iostream>

#include "stb_image.h"

CubeMap::CubeMap(DynArr<const char*>& facePaths) : Texture(0, 0, 0)
{
	m_data = nullptr;

	m_nWidth = 0;
	m_nHeight = 0;
	m_nChannels = 0;
	m_glHandle = 0;
	m_bOwnsTexture = true;

	// Load side images...
	DynArr<unsigned char*> imageData;
	for (int i = 0; i < facePaths.Count(); ++i)
		imageData.Push(nullptr);

	bool bLoadSuccessful = true;

	for (int i = 0; i < imageData.Count() && bLoadSuccessful; ++i) 
	{
		imageData[i] = stbi_load(facePaths[i], &m_nWidth, &m_nHeight, &m_nChannels, STBI_rgb);

		bLoadSuccessful &= imageData[i] != nullptr;

		if(!bLoadSuccessful) 
		{
			std::cout << "Failed to load cubemap image: " << facePaths[i] << std::endl;
			return;
		}
		else
			std::cout << "Successfully loaded cubemap image: " << facePaths[i] << std::endl;
	}

	// Create and bind texture buffer.s
	glGenTextures(1, &m_glHandle);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_glHandle);

	// Send data to buffers...
	for (int i = 0; i < imageData.Count(); ++i) 
	{
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, m_nWidth, m_nHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData[i]);

		stbi_image_free(imageData[i]);
	}

	glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

	// Specify texture parameters...

	// Texture wrapping
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// Texture filtering
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR); // Nearest neighbour on shrink.
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR); // Nearest neighbour on expand.

	// Unbind texture when finished.
	glBindTexture(GL_TEXTURE_2D, 0);
}

CubeMap::~CubeMap()
{

}

void CubeMap::BindCubeMap() 
{
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_glHandle);
}
