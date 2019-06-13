#pragma once
#include "Texture.h"
#include <string>
#include "DynamicArray.h"

class CubeMap : public Texture
{
public:

	CubeMap(DynArr<const char*>& facePaths);

	~CubeMap();

	void BindCubeMap();

private:

};