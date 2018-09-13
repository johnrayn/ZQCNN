#include "ZQ_CNN_Net.h"
#include <cblas.h>
#include <vector>
#include <iostream>
#include "opencv2\opencv.hpp"
using namespace ZQ;
using namespace std;
using namespace cv;
int main()
{
	int num_threads = 1;
	openblas_set_num_threads(num_threads);
	
	std::string out_blob_name = "fc5";
	ZQ_CNN_Net net;
	//if (!net.LoadFrom("model\\model-r100-am.zqparams", "model\\model-r100-am.nchwbin", false))
	if (!net.LoadFrom("model\\mobilefacenet-res4-8-16-4-dim256.zqparams", "model\\mobilefacenet-res4-8-16-4-dim256-emore.nchwbin", false))
	//if (!net.LoadFrom("model\\mobilefacenet-v112X96.zqparams", "model\\mobilefacenet-v112X96.nchwbin",false))
	//if (!net.LoadFrom("model\\mobilefacenet-res2-6-10-2-dim256.zqparams", "model\\mobilefacenet-res2-6-10-2-dim256-emore.nchwbin", false))
		//if (!net.LoadFrom("model\\test.zqparams", "model\\test.nchwbin"))
		//if (!net.LoadFrom("model\\model-r50-am.zqparams", "model\\model-r50-am.nchwbin"))
		//if (!net.LoadFrom("model\\mobilenet_sphereface10bn512.zqparams", "model\\mobilenet_sphereface10bn512_iter_50000.nchwbin"))
		//if (!net.LoadFrom("model\\sphereface04bn256.zqparams", "model\\sphereface04bn256_iter_26000.nchwbin"))
		//if (!net.LoadFrom("model\\sphereface20.zqparams", "model\\sphereface20.nchwbin"))
		//if (!net.LoadFrom("model\\sphereface04.zqparams", "model\\sphereface04.nchwbin"))
		//if (!net.LoadFrom("model\\sphereface06bn.zqparams", "model\\sphereface06bn_iter_80000.nchwbin"))
	{
		cout << "failed to load net\n";
		return EXIT_FAILURE;
	}

	const ZQ_CNN_Tensor4D* ptr = net.GetBlobByName(out_blob_name);
	if (ptr == 0)
	{
		cout << "The blob " << out_blob_name << " does not exist!\n";
		return EXIT_FAILURE;
	}

	int input_H, input_W, input_C;
	net.GetInputDim(input_C, input_H, input_W);

	Mat image0, image1;
	if (input_C == 3 && input_H == 112 && input_W == 112)
	{
		std::string name = "data\\00_.jpg";
		image0 = cv::imread(name, 1);
		if (image0.empty())
		{
			cout << name << " does not exist!\n";
			return EXIT_FAILURE;
		}
		name = "data\\01_.jpg";
		image1 = cv::imread(name, 1);
		if (image1.empty())
		{
			cout << name << " does not exist!\n";
			return EXIT_FAILURE;
		}
	}
	else if(input_C == 3 && input_H == 112 && input_W == 96)
	{
		std::string name = "data\\00.jpg";
		image0 = cv::imread(name, 1);
		if (image0.empty())
		{
			cout << name << " does not exist!\n";
			return EXIT_FAILURE;
		}
		name = "data\\01.jpg";
		image1 = cv::imread(name, 1);
		if (image1.empty())
		{
			cout << name << " does not exist!\n";
			return EXIT_FAILURE;
		}
	}
	else
	{
		cout << "unsupported resolution: WxHxC = " << input_W << "x" << input_H << "x" << input_C << "\n";
		return EXIT_FAILURE;
	}

	//net.TurnOnShowDebugInfo();
	for (int out_it = 0; out_it < 10; out_it++)
	{
		ZQ_CNN_Tensor4D_NHW_C_Align128bit input0, input1;
		input0.ConvertFromBGR(image0.data, image0.cols, image0.rows, image0.step[0]);
		input1.ConvertFromBGR(image1.data, image1.cols, image1.rows, image1.step[0]);

		printf("num_MulAdd: %.3f M\n", net.GetNumOfMulAdd() / (1024.0*1024.0));
		int iters = 100;
		double t1 = omp_get_wtime();
		for (int it = 0; it < iters; it++)
		{
			double t3 = omp_get_wtime();
			if (!net.Forward(input0, 1))
			{
				cout << "failed to run\n";
				return EXIT_FAILURE;
			}
			double t4 = omp_get_wtime();
			//printf("forward costs: %.3f ms\n", 1000 * (t4 - t3));
		}
		double t2 = omp_get_wtime();
		printf("[%d] times cost %.3f s, 1 iter cost %.3f ms\n", iters, t2 - t1, 1000 * (t2 - t1) / iters);

		ptr = net.GetBlobByName(out_blob_name);
		int dim = ptr->GetC();
		std::vector<float> feat0(dim);
		memcpy(&feat0[0], ptr->GetFirstPixelPtr(), sizeof(float)*dim);


		double t3 = omp_get_wtime();
		for (int it = 0; it < iters; it++)
		{
			if (!net.Forward(input1, 1))
			{
				cout << "failed to run\n";
				return EXIT_FAILURE;
			}
		}
		double t4 = omp_get_wtime();
		printf("[%d] times cost %.3f s, 1 iter cost %.3f ms\n", iters, t4 - t3, 1000 * (t4 - t3) / iters);

		ptr = net.GetBlobByName(out_blob_name);
		std::vector<float> feat1(dim);
		memcpy(&feat1[0], ptr->GetFirstPixelPtr(), sizeof(float)*dim);
		float score = 0;
		float len0 = 0, len1 = 0;
		for (int i = 0; i < dim; i++)
		{
			score += feat0[i] * feat1[i];
			len0 += feat0[i] * feat0[i];
			len1 += feat1[i] * feat1[i];
		}
		len0 = sqrt(len0);
		len1 = sqrt(len1);
		score /= (len0*len1 + 1e-64);
		for (int i = 0; i < dim; i++)
		{
			feat0[i] /= len0;
			feat1[i] /= len1;
		}
		//std::cout << "feat0[0] = " << feat0[0] << "\n";
		//std::cout << "feat1[0] = " << feat1[0] << "\n";
		std::cout << "Similarity score: " << score << "\n";
	}
	return EXIT_SUCCESS;
}
