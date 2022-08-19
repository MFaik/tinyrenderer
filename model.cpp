#include <iostream>
#include <string>
#include <fstream>
#include <sstream>
#include <vector>
#include "model.h"

Model::Model(const char *filename) : verts_(), textureVerts_(), normals_(), faces_() {
    std::ifstream in;
    in.open (filename, std::ifstream::in);
    if (in.fail()) return;
    std::string line;
    while (!in.eof()) {
        std::getline(in, line);
        std::istringstream iss(line.c_str());
        char trash;
        if (!line.compare(0, 2, "v ")) {
            iss >> trash;
            Vec3f v;
            for (int i=0;i<3;i++) iss >> v[i];
            verts_.push_back(v);
        } else if(!line.compare(0, 3, "vt ")){
            iss >> trash >> trash;
            Vec3f vt;
            for (int i=0;i<3;i++) iss >> vt[i];
            textureVerts_.push_back(vt);
        } else if(!line.compare(0, 3, "vn ")){
            iss >> trash >> trash;
            Vec3f vn;
            for (int i=0;i<3;i++) iss >> vn[i];
            normals_.push_back(vn);
        } else if (!line.compare(0, 2, "f ")) {
            std::vector<std::array<int, 3>> f;
            int vertexid, textureid, normalid;
            iss >> trash;
            while (iss >> vertexid >> trash >> textureid >> trash >> normalid) {
                // in wavefront obj all indices start at 1, not zero
                vertexid--;
                textureid--;
                normalid--;
                f.push_back({vertexid, textureid, normalid});
            }
            faces_.push_back(f);
        }
    }
    std::cerr << "# v# " << verts_.size() << " f# "  << faces_.size() << std::endl;
}

Model::~Model() {
}

int Model::nverts() {
    return (int)verts_.size();
}

int Model::nfaces() {
    return (int)faces_.size();
}

std::vector<std::array<int, 3>> Model::face(int idx) {
    return faces_[idx];
}

Vec3f Model::vert(int i) {
    return verts_[i];
}

Vec3f Model::textureVert(int i) {
    return textureVerts_[i];
}

Vec3f Model::normal(int i) {
    return normals_[i];
}