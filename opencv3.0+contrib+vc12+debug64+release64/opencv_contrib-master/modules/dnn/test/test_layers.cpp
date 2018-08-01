/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                           License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2013, OpenCV Foundation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of the copyright holders may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#include "test_precomp.hpp"
#include <opencv2/core/ocl.hpp>
#include <iostream>
#include "npy_blob.hpp"
#include <opencv2/dnn/all_layers.hpp>
#include <opencv2/ts/ocl_test.hpp>

namespace cvtest
{

using namespace cv;
using namespace cv::dnn;

template<typename TString>
static String _tf(TString filename)
{
    String basetestdir = getOpenCVExtraDir();
    size_t len = basetestdir.size();
    if(len > 0 && basetestdir[len-1] != '/' && basetestdir[len-1] != '\\')
        return (basetestdir + "/dnn/layers") + filename;
    return (basetestdir + "dnn/layers/") + filename;
}

void runLayer(Ptr<Layer> layer, std::vector<Mat> &inpBlobs, std::vector<Mat> &outBlobs)
{
    size_t i, ninputs = inpBlobs.size();
    std::vector<Mat> inp_(ninputs);
    std::vector<Mat*> inp(ninputs);
    std::vector<Mat> outp;

    for( i = 0; i < ninputs; i++ )
    {
        inp_[i] = inpBlobs[i].clone();
        inp[i] = &inp_[i];
    }

    layer->allocate(inp, outp);
    layer->forward(inp, outp);

    size_t noutputs = outp.size();
    outBlobs.resize(noutputs);
    for( i = 0; i < noutputs; i++ )
        outBlobs[i] = outp[i];
}


void testLayerUsingCaffeModels(String basename, bool useCaffeModel = false, bool useCommonInputBlob = true)
{
    String prototxt = _tf(basename + ".prototxt");
    String caffemodel = _tf(basename + ".caffemodel");

    String inpfile = (useCommonInputBlob) ? _tf("blob.npy") : _tf(basename + ".input.npy");
    String outfile = _tf(basename + ".npy");

    cv::setNumThreads(cv::getNumberOfCPUs());

    Net net;
    {
        Ptr<Importer> importer = createCaffeImporter(prototxt, (useCaffeModel) ? caffemodel : String());
        ASSERT_TRUE(importer != NULL);
        importer->populateNet(net);
    }

    Mat inp = blobFromNPY(inpfile);
    Mat ref = blobFromNPY(outfile);

    net.setBlob(".input", inp);
    net.forward();
    Mat out = net.getBlob("output");

    normAssert(ref, out);
}

TEST(Layer_Test_Softmax, Accuracy)
{
     testLayerUsingCaffeModels("layer_softmax");
}

TEST(Layer_Test_LRN_spatial, Accuracy)
{
     testLayerUsingCaffeModels("layer_lrn_spatial");
}

TEST(Layer_Test_LRN_channels, Accuracy)
{
     testLayerUsingCaffeModels("layer_lrn_channels");
}

TEST(Layer_Test_Convolution, Accuracy)
{
     testLayerUsingCaffeModels("layer_convolution", true);
}

TEST(Layer_Test_DeConvolution, Accuracy)
{
     testLayerUsingCaffeModels("layer_deconvolution", true, false);
}

TEST(Layer_Test_InnerProduct, Accuracy)
{
     testLayerUsingCaffeModels("layer_inner_product", true);
}

TEST(Layer_Test_Pooling_max, Accuracy)
{
     testLayerUsingCaffeModels("layer_pooling_max");
}

TEST(Layer_Test_Pooling_ave, Accuracy)
{
     testLayerUsingCaffeModels("layer_pooling_ave");
}

TEST(Layer_Test_MVN, Accuracy)
{
     testLayerUsingCaffeModels("layer_mvn");
}

TEST(Layer_Test_Reshape, squeeze)
{
    LayerParams params;
    params.set("axis", 2);
    params.set("num_axes", 1);

    int sz[] = {4, 3, 1, 2};
    Mat inp(4, sz, CV_32F);
    std::vector<Mat*> inpVec(1, &inp);
    std::vector<Mat> outVec;

    Ptr<Layer> rl = LayerFactory::createLayerInstance("Reshape", params);
    rl->allocate(inpVec, outVec);
    rl->forward(inpVec, outVec);

    Mat& out = outVec[0];
    std::vector<int> shape(out.size.p, out.size.p + out.dims);
    int sh0[] = {4, 3, 2};
    std::vector<int> shape0(sh0, sh0+3);
    EXPECT_TRUE(shapeEqual(shape, shape0));
}

TEST(Layer_Test_BatchNorm, Accuracy)
{
     testLayerUsingCaffeModels("layer_batch_norm", true);
}

//template<typename XMat>
//static void test_Layer_Concat()
//{
//    Matx21f a(1.f, 1.f), b(2.f, 2.f), c(3.f, 3.f);
//    std::vector<Blob> res(1), src = { Blob(XMat(a)), Blob(XMat(b)), Blob(XMat(c)) };
//    Blob ref(XMat(Matx23f(1.f, 2.f, 3.f, 1.f, 2.f, 3.f)));
//
//    runLayer(ConcatLayer::create(1), src, res);
//    normAssert(ref, res[0]);
//}
//TEST(Layer_Concat, Accuracy)
//{
//    test_Layer_Concat<Mat>());
//}
//OCL_TEST(Layer_Concat, Accuracy)
//{
//    OCL_ON(test_Layer_Concat<Mat>());
//    );
//}

static void test_Reshape_Split_Slice_layers()
{
    Net net;
    {
        Ptr<Importer> importer = createCaffeImporter(_tf("reshape_and_slice_routines.prototxt"));
        ASSERT_TRUE(importer != NULL);
        importer->populateNet(net);
    }

    Mat input(6, 12, CV_32F);
    RNG rng(0);
    rng.fill(input, RNG::UNIFORM, -1, 1);

    net.setBlob(".input", input);
    net.forward();
    Mat output = net.getBlob("output");

    normAssert(input, output);
}
TEST(Layer_Test_Reshape_Split_Slice, Accuracy)
{
    test_Reshape_Split_Slice_layers();
}

class Layer_LSTM_Test : public ::testing::Test
{
public:
    int numInp, numOut;
    Mat Wh, Wx, b;
    Ptr<LSTMLayer> layer;
    std::vector<Mat> inputs, outputs;

    Layer_LSTM_Test() {}

    void init(const std::vector<int> &inpShape_, const std::vector<int> &outShape_)
    {
        numInp = (int)shapeTotal(inpShape_);
        numOut = (int)shapeTotal(outShape_);

        Wh = Mat::ones(4 * numOut, numOut, CV_32F);
        Wx = Mat::ones(4 * numOut, numInp, CV_32F);
        b  = Mat::ones(4 * numOut, 1, CV_32F);

        layer = LSTMLayer::create(LayerParams());
        layer->setWeights(Wh, Wx, b);
        layer->setOutShape(outShape_);
    }
};

TEST_F(Layer_LSTM_Test, get_set_test)
{
    const int TN = 4;
    std::vector<int> inpShape = makeShape(5, 3, 2);
    std::vector<int> outShape = makeShape(3, 1, 2);
    std::vector<int> inpResShape = concatShape(makeShape(TN), inpShape);
    std::vector<int> outResShape = concatShape(makeShape(TN), outShape);

    init(inpShape, outShape);
    layer->setProduceCellOutput(true);
    layer->setUseTimstampsDim(false);
    layer->setOutShape(outShape);

    Mat C((int)outResShape.size(), &outResShape[0], CV_32F);
    randu(C, -1., 1.);
    Mat H = C.clone();
    randu(H, -1., 1.);
    layer->setC(C);
    layer->setH(H);

    Mat inp((int)inpResShape.size(), &inpResShape[0], CV_32F);
    randu(inp, -1., 1.);

    inputs.push_back(inp);
    runLayer(layer, inputs, outputs);

    EXPECT_EQ(2u, outputs.size());

    printShape("outResShape", outResShape);
    printShape("out0", getShape(outputs[0]));
    printShape("out1", getShape(outputs[0]));
    printShape("C", getShape(layer->getC()));
    printShape("H", getShape(layer->getH()));

    EXPECT_TRUE(shapeEqual(outResShape, getShape(outputs[0])));
    EXPECT_TRUE(shapeEqual(outResShape, getShape(outputs[1])));

    EXPECT_TRUE(shapeEqual(outResShape, getShape(layer->getC())));
    EXPECT_TRUE(shapeEqual(outResShape, getShape(layer->getH())));

    EXPECT_EQ(0, layer->inputNameToIndex("x"));
    EXPECT_EQ(0, layer->outputNameToIndex("h"));
    EXPECT_EQ(1, layer->outputNameToIndex("c"));
}

TEST(Layer_LSTM_Test_Accuracy_with_, CaffeRecurrent)
{
    Ptr<LSTMLayer> layer = LSTMLayer::create(LayerParams());

    Mat Wx = blobFromNPY(_tf("lstm.prototxt.w_0.npy"));
    Mat Wh = blobFromNPY(_tf("lstm.prototxt.w_2.npy"));
    Mat b  = blobFromNPY(_tf("lstm.prototxt.w_1.npy"));
    layer->setWeights(Wh, Wx, b);

    Mat inp = blobFromNPY(_tf("recurrent.input.npy"));
    std::vector<Mat> inputs(1, inp), outputs;
    runLayer(layer, inputs, outputs);

    Mat h_t_reference = blobFromNPY(_tf("lstm.prototxt.h_1.npy"));
    normAssert(h_t_reference, outputs[0]);
}

TEST(Layer_RNN_Test_Accuracy_with_, CaffeRecurrent)
{
    Ptr<RNNLayer> layer = RNNLayer::create(LayerParams());

    layer->setWeights(
                blobFromNPY(_tf("rnn.prototxt.w_0.npy")),
                blobFromNPY(_tf("rnn.prototxt.w_1.npy")),
                blobFromNPY(_tf("rnn.prototxt.w_2.npy")),
                blobFromNPY(_tf("rnn.prototxt.w_3.npy")),
                blobFromNPY(_tf("rnn.prototxt.w_4.npy")) );

    std::vector<Mat> output, input(1, blobFromNPY(_tf("recurrent.input.npy")));
    runLayer(layer, input, output);

    Mat h_ref = blobFromNPY(_tf("rnn.prototxt.h_1.npy"));
    normAssert(h_ref, output[0]);
}


class Layer_RNN_Test : public ::testing::Test
{
public:
    int nX, nH, nO, nT, nS;
    Mat Whh, Wxh, bh, Who, bo;
    Ptr<RNNLayer> layer;

    std::vector<Mat> inputs, outputs;

    Layer_RNN_Test()
    {
        nT = 3;
        nS = 5;
        nX = 31;
        nH = 64;
        nO = 100;

        Whh = Mat::ones(nH, nH, CV_32F);
        Wxh = Mat::ones(nH, nX, CV_32F);
        bh  = Mat::ones(nH, 1, CV_32F);
        Who = Mat::ones(nO, nH, CV_32F);
        bo  = Mat::ones(nO, 1, CV_32F);

        layer = RNNLayer::create(LayerParams());
        layer->setProduceHiddenOutput(true);
        layer->setWeights(Wxh, bh, Whh, Who, bo);
    }
};

TEST_F(Layer_RNN_Test, get_set_test)
{
    int sz[] = { nT, nS, 1, nX };
    Mat inp(4, sz, CV_32F);
    randu(inp, -1., 1.);
    inputs.push_back(inp);
    runLayer(layer, inputs, outputs);

    EXPECT_EQ(outputs.size(), 2u);
    EXPECT_TRUE(shapeEqual(getShape(outputs[0]), makeShape(nT, nS, nO)));
    EXPECT_TRUE(shapeEqual(getShape(outputs[1]), makeShape(nT, nS, nH)));
}

}
