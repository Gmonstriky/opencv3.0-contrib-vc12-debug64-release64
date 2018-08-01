// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

// Copyright (C) 2016, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.

/*
Sample of using OpenCV dnn module with Tensorflow Inception model.
*/

#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace cv::dnn;

#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace std;

const String keys =
        "{help h    || Sample app for loading Inception TensorFlow model. "
                       "The model and class names list can be downloaded here: "
                       "https://storage.googleapis.com/download.tensorflow.org/models/inception5h.zip }"
        "{model m   |tensorflow_inception_graph.pb| path to TensorFlow .pb model file }"
        "{image i   || path to image file }"
        "{i_blob    | .input | input blob name) }"
        "{o_blob    | softmax2 | output blob name) }"
        "{c_names c | imagenet_comp_graph_label_strings.txt | path to file with classnames for class id }"
        "{result r  || path to save output blob (optional, binary format, NCHW order) }"
        ;

void getMaxClass(const Mat &probBlob, int *classId, double *classProb);
std::vector<String> readClassNames(const char *filename);

int main(int argc, char **argv)
{
    cv::CommandLineParser parser(argc, argv, keys);

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    String modelFile = parser.get<String>("model");
    String imageFile = parser.get<String>("image");
    String inBlobName = parser.get<String>("i_blob");
    String outBlobName = parser.get<String>("o_blob");

    if (!parser.check())
    {
        parser.printErrors();
        return 0;
    }

    String classNamesFile = parser.get<String>("c_names");
    String resultFile = parser.get<String>("result");

    //! [Create the importer of TensorFlow model]
    Ptr<dnn::Importer> importer;
    try                                     //Try to import TensorFlow AlexNet model
    {
        importer = dnn::createTensorflowImporter(modelFile);
    }
    catch (const cv::Exception &err)        //Importer can throw errors, we will catch them
    {
        std::cerr << err.msg << std::endl;
    }
    //! [Create the importer of Caffe model]

    if (!importer)
    {
        std::cerr << "Can't load network by using the mode file: " << std::endl;
        std::cerr << modelFile << std::endl;
        exit(-1);
    }

    //! [Initialize network]
    dnn::Net net;
    importer->populateNet(net);
    importer.release();                     //We don't need importer anymore
    //! [Initialize network]

    //! [Prepare blob]
    Mat img = imread(imageFile);
    if (img.empty())
    {
        std::cerr << "Can't read image from the file: " << imageFile << std::endl;
        exit(-1);
    }

    cv::Size inputImgSize = cv::Size(224, 224);

    if (inputImgSize != img.size())
        resize(img, img, inputImgSize);       //Resize image to input size

    Mat inputBlob = blobFromImage(img);   //Convert Mat to image batch
    //! [Prepare blob]
    inputBlob -= 117.0;
    //! [Set input blob]
    net.setBlob(inBlobName, inputBlob);        //set the network input
    //! [Set input blob]

    cv::TickMeter tm;
    tm.start();

    //! [Make forward pass]
    net.forward();                          //compute output
    //! [Make forward pass]

    tm.stop();

    //! [Gather output]
    Mat result = net.getBlob(outBlobName);   //gather output of "prob" layer

    if (!resultFile.empty()) {
        CV_Assert(result.isContinuous());

        ofstream fout(resultFile.c_str(), ios::out | ios::binary);
        fout.write((char*)result.data, result.total() * sizeof(float));
        fout.close();
    }

    std::cout << "Output blob shape " << result.size[0] << " x " << result.size[1] << " x " << result.size[2] << " x " << result.size[3] << std::endl;
    std::cout << "Inference time, ms: " << tm.getTimeMilli()  << std::endl;

    if (!classNamesFile.empty()) {
        std::vector<String> classNames = readClassNames(classNamesFile.c_str());

        int classId;
        double classProb;
        getMaxClass(result, &classId, &classProb);//find the best class

        //! [Print results]
        std::cout << "Best class: #" << classId << " '" << classNames.at(classId) << "'" << std::endl;
        std::cout << "Probability: " << classProb * 100 << "%" << std::endl;
    }
    return 0;
} //main


/* Find best class for the blob (i. e. class with maximal probability) */
void getMaxClass(const Mat &probBlob, int *classId, double *classProb)
{
    Mat probMat = probBlob.reshape(1, 1); //reshape the blob to 1x1000 matrix
    Point classNumber;

    minMaxLoc(probMat, NULL, classProb, NULL, &classNumber);
    *classId = classNumber.x;
}

std::vector<String> readClassNames(const char *filename)
{
    std::vector<String> classNames;

    std::ifstream fp(filename);
    if (!fp.is_open())
    {
        std::cerr << "File with classes labels not found: " << filename << std::endl;
        exit(-1);
    }

    std::string name;
    while (!fp.eof())
    {
        std::getline(fp, name);
        if (name.length())
            classNames.push_back( name );
    }

    fp.close();
    return classNames;
}
