#include <vector>
#include <cmath>
#include <cstdlib>
#include <limits>
#include "tgaimage.h"
#include "model.h"
#include "geometry.h"

const TGAColor white = TGAColor(255, 255, 255, 255);
const TGAColor red   = TGAColor(255, 0,   0,   255);
Model *model = NULL;
const int width  = 800;
const int height = 800;
const int depth = 255;
const float c = 4;

void line(Vec2i p0, Vec2i p1, TGAImage &image, TGAColor color) {
    bool swapped = false;
    if(std::abs(p0.x-p1.x) < std::abs(p0.y-p1.y)){
        std::swap(p0.x, p0.y);
        std::swap(p1.x, p1.y);
        swapped = true;
    }
    if(p0.x > p1.x){
        std::swap(p0, p1);
    }

    for(int x = p0.x; x <= p1.x; x++){
        float t = (float)(x-p0.x)/(p1.x-p0.x);
        int y = p0.y + (p1.y-p0.y)*t;
        if(swapped){
            image.set(y, x, color);
        } else {
            image.set(x, y, color);
        }
    }
}

inline TGAColor pixelColor(float w1, float w2, Vec3f *texture_pts, TGAImage &diffuse) {
	Vec3f v1 = texture_pts[1] - texture_pts[0];
	Vec3f v2 = texture_pts[2] - texture_pts[0];
	Vec3f p  = texture_pts[0] + v1*w1 + v2*w2; 
	return diffuse.get(p.x * diffuse.get_width(), p.y * diffuse.get_height());
}

void triangle(Vec3f *pts, Vec3f *texture_pts, float *zbuffer, TGAImage &image, TGAImage &diffuse, float intensity) {
	// have to swap because of zero division issues
	if(pts[2].y == pts[0].y){
		std::swap(pts[0], pts[1]);
		std::swap(pts[0], pts[2]);
		
		std::swap(texture_pts[0], texture_pts[1]);
		std::swap(texture_pts[0], texture_pts[2]);
	}
	Vec2f bboxmax(0,0);
	Vec2f bboxmin(image.get_width()-1,image.get_height()-1);
	Vec2f clamp(image.get_width()-1,image.get_height()-1); 
	for(int i = 0; i < 3; i++){
		for(int j = 0;j < 2;j++){
			bboxmax[j] = std::min(clamp[j], std::max(bboxmax[j], pts[i][j]));
			bboxmin[j] = std::max(0.0f, std::min(bboxmin[j], pts[i][j]));
		}
	}
	for(int y = bboxmin.y; y <= bboxmax.y; y++){
		for(int x = bboxmin.x; x <= bboxmax.x; x++){
			Vec3f v1 = pts[1] - pts[0];
			Vec3f v2 = pts[2] - pts[0];
			float w1 = (pts[0].x*v2.y + (y-pts[0].y)*v2.x - x*v2.y)/(v1.y*v2.x - v1.x*v2.y);
			float w2 = (y - pts[0].y - w1*v1.y)/v2.y;
			float z = pts[0].z + v1.z*w1 + v2.z*w2;
			if(w1 >= -1e-4 && w2 >= -1e-4 && w1+w2 < 1+1e-4){
				if(zbuffer[x+y*image.get_width()] < z){
					zbuffer[x+y*image.get_width()] = z;
					image.set(x, y, pixelColor(w1, w2, texture_pts, diffuse) * intensity);
				}
			}
		}
	}
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

int main(int argc, char** argv) {
    if(2==argc){
        model = new Model(argv[1]);
    } else {
        model = new Model("obj/african_head/african_head.obj");
    }

    float *zbuffer = new float[width*height];
    for(int i = 0;i < width*height;i++)
		zbuffer[i] = -std::numeric_limits<float>::max();

	TGAImage diffuse;
	diffuse.read_tga_file("obj/african_head/african_head_diffuse.tga");
	diffuse.flip_vertically();

	Matrix m1 = Matrix::identity(4);
	m1[0][3] = 1;
	m1[1][3] = 1;
	Matrix m2 = Matrix::identity(4);
	m2[0][0] = width/2.0f;
	m2[1][1] = height/2.0f;
	Matrix m3 = Matrix::identity(4);
	m3[3][2] = -1/c;
	Matrix projection = m2*m1*m3;

    TGAImage image(width, height, TGAImage::RGB);
    for(int i = 0; i < model->nfaces(); i++) {
        auto face = model->face(i);
        Vec3f pts[3];
        for(int i = 0; i < 3; i++) pts[i] = model->vert(face[i][0]);
		Vec3f normal = ((pts[1]-pts[0])^(pts[2]-pts[1])).normalize();
		if(normal.z > 0){
			for(int i = 0; i < 3; i++) {
				pts[i] = projection*Matrix(pts[i]);
			}
			Vec3f texture_pts[3];
			for(int i = 0; i < 3; i++) texture_pts[i] = model->textureVert(face[i][1]);
			triangle(pts, texture_pts, zbuffer, image, diffuse, normal.z); 
		}
    }
	// Vec3f pts[]{Vec3f(120.000000,600.000000,0.466332),Vec3f(332.000000,600.000000,0.415616),Vec3f(314.000000,600.000000,0.455218)};
	// triangle(pts, zbuffer, image, TGAColor(255));

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

