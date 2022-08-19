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
const float c = 10;
Vec3f light_dir = Vec3f(0,1,1).normalize();

inline TGAColor pixelColor(float w1, float w2, Vec3f *texture_pts, Vec3f *vertex_normals, TGAImage &diffuse) {
	Vec3f pdiff1 = texture_pts[1] - texture_pts[0];
	Vec3f pdiff2 = texture_pts[2] - texture_pts[0];
	Vec3f p  = texture_pts[0] + pdiff1*w1 + pdiff2*w2;

	Vec3f ndiff1 = vertex_normals[1] - vertex_normals[0];
	Vec3f ndiff2 = vertex_normals[2] - vertex_normals[0];
	Vec3f n = vertex_normals[0];
	float intensity = (n + ndiff1*w1 + ndiff2*w2)*light_dir;
	
	return diffuse.get(p.x * diffuse.get_width(), p.y * diffuse.get_height()) * intensity;
}

void triangle(Vec3f *pts, Vec3f *texture_pts, Vec3f *vertex_normals, float *zbuffer, TGAImage &image, TGAImage &diffuse) {
	// have to swap because of zero division issues
	if(pts[2].y == pts[0].y){
		std::swap(pts[0], pts[1]);
		std::swap(pts[0], pts[2]);
		
		std::swap(texture_pts[0], texture_pts[1]);
		std::swap(texture_pts[0], texture_pts[2]);

		std::swap(vertex_normals[0],vertex_normals[1]);
		std::swap(vertex_normals[0],vertex_normals[2]);
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
					image.set(x, y, pixelColor(w1, w2, texture_pts, vertex_normals, diffuse));
				}
			}
		}
	}
}

Vec3f world2screen(Vec3f v) {
    return Vec3f(int((v.x+1.)*width/2.+.5), int((v.y+1.)*height/2.+.5), v.z);
}

Matrix lookAt(Vec3f eye, Vec3f center, Vec3f up) {
	Vec3f k = (eye-center).normalize();
	Vec3f i = (up ^ k).normalize();
	Vec3f j = (k ^ i).normalize();
	Matrix m = Matrix::identity(4);
	Matrix tr = Matrix::identity(4);
	for(int r = 0;r < 3;r++){
		// m[r][0] = i[r];
		// m[r][1] = j[r];
		// m[r][2] = k[r];

		m[0][r] = i[r];
		m[1][r] = j[r];
		m[2][r] = k[r];

		tr[r][3] = -eye[r];
	}

	return m*tr;
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

	Matrix view = Matrix::identity(4);
	view = lookAt(Vec3f(-1,1,3),Vec3f(0,0,0),Vec3f(0,1,0));

	Matrix projection = Matrix::identity(4);//perspective projection
	projection[3][2] = -1/c;
	Matrix m1 = Matrix::identity(4);//get to the middle of the screen
	m1[0][3] = 1;
	m1[1][3] = 1;
	m1[2][3] = 1;
	Matrix m2 = Matrix::identity(4);//resize to screen pixel size
	m2[0][0] = width/2.0f;
	m2[1][1] = height/2.0f;
	m2[2][2] = depth/2.0f;
	Matrix viewPort = m2*m1;

    TGAImage image(width, height, TGAImage::RGB);
    for(int i = 0; i < model->nfaces(); i++) {
        auto face = model->face(i);
        Vec3f pts[3];
        for(int i = 0; i < 3; i++) pts[i] = projection*view*Matrix(model->vert(face[i][0]));
		Vec3f projectedNormal = ((pts[1]-pts[0])^(pts[2]-pts[1])).normalize();
		if(projectedNormal.z > 0){
			for(int i = 0; i < 3; i++) pts[i] = viewPort*Matrix(pts[i]);
			Vec3f texture_pts[3];
			for(int i = 0; i < 3; i++) texture_pts[i] = model->textureVert(face[i][1]);
			Vec3f vertex_normals[3];
			for(int i = 0; i < 3; i++) vertex_normals[i] = model->normal(face[i][2]);
			triangle(pts, texture_pts, vertex_normals,zbuffer, image, diffuse); 
		}
	}

    image.flip_vertically(); // i want to have the origin at the left bottom corner of the image
    image.write_tga_file("output.tga");
    delete model;
    return 0;
}

