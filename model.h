#ifndef __MODEL_H__
#define __MODEL_H__

#include <vector>
#include <array>
#include "geometry.h"

class Model {
private:
	std::vector<Vec3f> verts_;
	std::vector<Vec3f> textureVerts_;
	std::vector<std::vector<std::array<int,3> > > faces_;
public:
	Model(const char *filename);
	~Model();
	int nverts();
	int nfaces();
	Vec3f vert(int i);
	Vec3f textureVert(int i);
	std::vector<std::array<int,3>> face(int idx);
};

#endif //__MODEL_H__
