// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

// Copyright (C) 2016, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.

/*
Test for Tensorflow models loading
*/

#include "test_precomp.hpp"
#include "npy_blob.hpp"

namespace cvtest
{

using namespace cv;
using namespace cv::dnn;

template<typename TString>
static std::string _tf(TString filename)
{
    return (getOpenCVExtraDir() + "/dnn/") + filename;
}

TEST(Test_TensorFlow, read_inception)
{
    Net net;
    {
        const string model = findDataFile("dnn/tensorflow_inception_graph.pb", false);
        Ptr<Importer> importer = createTensorflowImporter(model);
        ASSERT_TRUE(importer != NULL);
        importer->populateNet(net);
    }

    Mat sample = imread(_tf("grace_hopper_227.png"));
    ASSERT_TRUE(!sample.empty());
    Mat input;
    resize(sample, input, Size(224, 224));
    input -= 128; // mean sub

    Mat inputBlob = blobFromImage(input, 1.);

    net.setBlob("_input.input", inputBlob);
    net.forward();

    Mat out = net.getBlob("softmax2");
    std::cout << out.dims << std::endl;
}

TEST(Test_TensorFlow, inception_accuracy)
{
    Net net;
    {
        const string model = findDataFile("dnn/tensorflow_inception_graph.pb", false);
        Ptr<Importer> importer = createTensorflowImporter(model);
        ASSERT_TRUE(importer != NULL);
        importer->populateNet(net);
    }

    Mat sample = imread(_tf("grace_hopper_227.png"));
    ASSERT_TRUE(!sample.empty());
    resize(sample, sample, Size(224, 224));
    Mat inputBlob = blobFromImage(sample, 1.);

    net.setBlob(".input", inputBlob);
    net.forward();

    Mat out = net.getBlob("softmax2");
    Mat ref = blobFromNPY(_tf("tf_inception_prob.npy"));

    normAssert(ref, out);
}

}
