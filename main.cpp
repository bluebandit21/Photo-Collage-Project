/*
 * Stitch.cpp
 *
 *  Created on: Jan 8, 2019
 *      Author: plotner
 */




#include <highgui.hpp>
#include <core.hpp>
#include <iostream>
#include <vector>
#include <fstream>

#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <cstring>

using namespace std;
using namespace cv;

typedef vector<vector<vector<uint8_t> > > image_vector;

struct IPair {string path; vector<uint8_t> average;};
struct FOutput {string path; unsigned int fitness;};
struct DNode{
	string path;
	int g_size;
	image_vector decomposition;
	DNode(string _path, int _g_size){
		path=_path;
		g_size=_g_size;
	}
	DNode(string _path, int _g_size, image_vector _decomposition){
		path=_path;
		g_size=_g_size;
		decomposition=_decomposition;
	}
	inline bool operator==(DNode& comp){
			return comp.path==path && comp.g_size==g_size;
	};
};
struct DCache{
	int g_size;
	vector<DNode*> nodes;
	DCache(int _g_size){
		g_size=_g_size;
	}
	void Add_Node(DNode* node){
		nodes.push_back(node);
	}
	bool contains(DNode* node){
		for(auto& i: nodes){
			if(*i==*node){
				node->decomposition=i->decomposition;
				return true;
			}
		}
		return false;
	}
};

int getdir (string dir, vector<string> &files){
    DIR *dp;
    struct dirent *dirp;
    if((dp  = opendir(dir.c_str())) == NULL) {
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }

    while ((dirp = readdir(dp)) != NULL) {
        files.push_back(string(dirp->d_name));
    }
    closedir(dp);
    return 0;
}

image_vector decompose(string path, int g_size=1){
	Mat image=cv::imread(path, CV_LOAD_IMAGE_COLOR);
	if(image.data==NULL){
		image_vector temp;
		return temp;
	}
	vector<Mat> channels;
	cv::split(image,channels);

		Mat blue_channel,red_channel,green_channel;
		blue_channel=channels[0];
		green_channel=channels[1];
		red_channel=channels[2];

		double curr_b_sum=0, curr_g_sum=0, curr_r_sum=0;
		image_vector output (g_size,vector<vector<uint8_t> >(g_size,vector <uint8_t>(3)));
		int h_step=image.rows/g_size;
		int w_step=image.cols/g_size;
		for(int i=0;i<g_size;i++){
			for(int j=0;j<g_size;j++){
				for(int row=0; row<h_step; row++){
					 uchar* b_p =  blue_channel.ptr(row+h_step*i); //pointer p points to the first place of each row
					 uchar* g_p =  green_channel.ptr(row+h_step*i);
					 uchar* r_p =  red_channel.ptr(row+h_step*i);
					 for(int it=0;it<w_step*j;it++){
						 b_p++;
						 g_p++;
						 r_p++;
					 }
					 for(int col = 0; col < w_step; col++) {
						curr_b_sum+=*b_p;
						curr_g_sum+=*g_p;
						curr_r_sum+=*r_p;
						b_p++;
					 	g_p++;
					 	r_p++;
					 }


				}
				output[i][j][0]=curr_b_sum / w_step / h_step;
				output[i][j][1]=curr_g_sum / w_step / h_step;
				output[i][j][2]=curr_r_sum / w_step / h_step;
				curr_b_sum=0;
				curr_g_sum=0;
				curr_r_sum=0;
			}
		}
		return output;

}
image_vector stitch(vector<image_vector> vectors, int g_size, int c_size){
	image_vector output (g_size*c_size,vector<vector<uint8_t> >(g_size*c_size,vector <uint8_t>(3)));
	for(int ci=0;ci<c_size;ci++){
		for(int cj=0;cj<c_size;cj++){
			for(int gi=0;gi<g_size;gi++){
				for(int gj=0;gj<g_size;gj++){
					output[ci*g_size+gi][cj*g_size+gj][0]=vectors[ci*c_size+cj][gi][gj][0];
					output[ci*g_size+gi][cj*g_size+gj][1]=vectors[ci*c_size+cj][gi][gj][1];
					output[ci*g_size+gi][cj*g_size+gj][2]=vectors[ci*c_size+cj][gi][gj][2];
				}
			}
		}
	}
	return output;
}
FOutput match(vector<IPair> images, vector<uint8_t> goal){
	string best_path="";
	unsigned int best_fitness = -1;
	for(auto &i: images){
		unsigned int curr_fitness = (i.average[0]-goal[0])*(i.average[0]-goal[0])+(i.average[1]-goal[1])*(i.average[1]-goal[1])+(i.average[2]-goal[2])*(i.average[2]-goal[2]);
		if(curr_fitness<best_fitness){
			best_path=i.path;
			best_fitness=curr_fitness;
		}
	}
	FOutput output={best_path,best_fitness};
	return output;
}
vector<FOutput> match_list(vector<IPair> images, image_vector* goal){
	vector<FOutput> output;
	for(auto& i: *goal){
		for(auto& pixel: i){
			output.push_back(match(images, pixel));
		}
	}
	return output;
}
vector<uint8_t> average(string path){
	Mat image;
	image=cv::imread(path, CV_LOAD_IMAGE_COLOR);
	if(image.data==NULL){
		vector<uint8_t> temp;
		return temp;
	}
	vector<Mat> channels;
	cv::split(image,channels);


	Mat blue_channel,red_channel,green_channel;
	blue_channel=channels[0];
	green_channel=channels[1];
	red_channel=channels[2];
	double b_sum=0, g_sum=0,r_sum=0;
	for(int row = 0; row < image.rows; ++row) {
	    uchar* b_p =  blue_channel.ptr(row); //pointer p points to the first place of each row
	    uchar* g_p =  green_channel.ptr(row);
	    uchar* r_p =  red_channel.ptr(row);

	    for(int col = 0; col < image.cols; ++col) {
	       	b_p++;
	       	g_p++;
	       	r_p++;
	       	b_sum+=*b_p;
	       	g_sum+=*g_p;
	       	r_sum+=*r_p;

	    }
	}
	uint8_t b_avg=b_sum / image.rows / image.cols + 0.5;
	uint8_t g_avg=g_sum / image.rows / image.cols + 0.5;
	uint8_t r_avg=r_sum / image.rows / image.cols + 0.5;
	vector<uint8_t> output;
	output.push_back(b_avg);
	output.push_back(g_avg);
	output.push_back(r_avg);
	return output;
}
vector<IPair> average_list(string root, vector<string> paths){
	vector<IPair> output;
	if(root[root.length()-1]!='/'){
		root+='/';
	}
	for(auto& i: paths){
		if(i[0]=='.'){continue;}
		cout << "Path: " << root+i << endl;
		try {
		IPair temp={root+i,average(root+i)};
		if(temp.average.size()==0){
			cout << "Imread failed for " << root+i << endl;
			continue;
		}
		output.push_back(temp);
		} catch(...){}
	}
	return output;
}
vector<image_vector> decompose_list(vector<string> paths, int g_size=20){
	vector<image_vector> output;
	DCache cache=DCache(g_size);
	for(auto& i: paths){
		if(i[0]=='.'){continue;}
		DNode* node=new DNode(i,g_size);
		if(!cache.contains(node)){ //running cache.contains(node) sets node->decomposition to the value contained within the cache if it is present.
			cout << "Caching " << i << ":" << g_size << endl;
			node->decomposition=decompose(i,g_size);
			if(node->decomposition.size()==0){
				cout << "Imread failed for " << i << endl;
				continue;
			}
			cache.Add_Node(node);
		}
		output.push_back(node->decomposition);
	}
	return output;
}
Mat encode(image_vector image, int height, int width){
	Mat b_channel= Mat(height,width,CV_8U);
	Mat g_channel= Mat(height,width,CV_8U);
	Mat r_channel= Mat(height,width,CV_8U);
	for(int i=0;i<height;i++){
		for(int j=0;j<width;j++){
			b_channel.at<uchar>(i,j)=image[i][j][0];
			g_channel.at<uchar>(i,j)=image[i][j][1];
			r_channel.at<uchar>(i,j)=image[i][j][2];
		}
	}
	vector<Mat> channels;
	channels.push_back(b_channel);
	channels.push_back(g_channel);
	channels.push_back(r_channel);

	Mat merge=Mat(height,width,CV_8UC3);
	cv::merge(channels,merge);
	return merge;
}
void collage(string image_path, string images_dir, string output_path, int c_size=200, int g_size=50){
	vector<string> files=vector<string>();
	getdir(images_dir, files);
	cout << "Averaging list..." << endl;
	vector<IPair> avg_list=average_list(images_dir, files);
	cout << "Decomposing source image..." << endl;
	image_vector temp=decompose(image_path,c_size);
	image_vector*goal=&temp;
	cout << "Generating matches..." << endl;
	vector<FOutput> o_list=match_list(avg_list, goal);
	vector<string> paths;
	unsigned int fitness=0;
	for(auto& i: o_list){
		paths.push_back(i.path);
		fitness+=i.fitness;
	}
	cout << "Average fitness: " << fitness / (c_size*c_size*g_size*g_size + 0.0) << endl;
	cout << "Paths extracted..." << endl;
	cout << "Decomposing lists..." << endl;
	vector<image_vector> d_list=decompose_list(paths, g_size); //This is a caching operation.
	cout << "Stitching images..." << endl;
	image_vector stitched=stitch(d_list, g_size, c_size);
	cout << "Encoding image..." << endl;
	Mat encoded=encode(stitched, g_size*c_size, g_size*c_size);
	cout << "Writing image to disk..." << endl;
	imwrite(output_path, encoded);
	namedWindow ("Test", WINDOW_NORMAL);
	imshow("Test", encoded);
	cout << "Press any key to continue." << endl;
	waitKey(0);
	return;
}

int main(int argc, char** argv){
	if(argc < 4 || argc > 6){
		cout << "Usage: source_image, image_directory, output_file, [c_size], [g_size]" << endl;
		return -1;
	}
	string source=argv[1];
	string directory=argv[2];
	string output_file=argv[3];
	int c_size=200;
	int g_size=50;
	if(argc>4){
		c_size=atoi(argv[4]);
	}
	if(argc>5){
		g_size=atoi(argv[5]);
	}
	collage(source,directory,output_file,c_size,g_size);

	return 0;

}
