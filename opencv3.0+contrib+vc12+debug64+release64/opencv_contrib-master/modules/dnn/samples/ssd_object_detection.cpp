#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace cv::dnn;

#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace std;

const size_t width = 300;
const size_t height = 300;

Mat getMean(const size_t& imageHeight, const size_t& imageWidth)
{
    Mat mean;

    const int meanValues[3] = {104, 117, 123};
    vector<Mat> meanChannels;
    for(size_t i = 0; i < 3; i++)
    {
        Mat channel(imageHeight, imageWidth, CV_32F, Scalar(meanValues[i]));
        meanChannels.push_back(channel);
    }
    cv::merge(meanChannels, mean);
    return mean;
}

Mat preprocess(const Mat& frame)
{
    Mat preprocessed;
    frame.convertTo(preprocessed, CV_32FC3);
    resize(preprocessed, preprocessed, Size(width, height)); //SSD accepts 300x300 RGB-images

    Mat mean = getMean(width, height);
    cv::subtract(preprocessed, mean, preprocessed);

    return preprocessed;
}

const char* about = "This sample uses Single-Shot Detector "
                    "(https://arxiv.org/abs/1512.02325)"
                    "to detect objects on image\n"; // TODO: link

const char* params
    = "{ help           | false | print usage         }"
      "{ proto          |       | model configuration }"
      "{ model          |       | model weights       }"
      "{ image          |       | image for detection }"
      "{ min_confidence | 0.5   | min confidence      }";

int main(int argc, char** argv)
{
    cv::CommandLineParser parser(argc, argv, params);

    if (parser.get<bool>("help"))
    {
        std::cout << about << std::endl;
        parser.printMessage();
        return 0;
    }

    cv::dnn::initModule();          //Required if OpenCV is built as static libs

    String modelConfiguration = parser.get<string>("proto");
    String modelBinary = parser.get<string>("model");

    //! [Create the importer of Caffe model]
    Ptr<dnn::Importer> importer;

    // Import Caffe SSD model
    try
    {
        importer = dnn::createCaffeImporter(modelConfiguration, modelBinary);
    }
    catch (const cv::Exception &err) //Importer can throw errors, we will catch them
    {
        cerr << err.msg << endl;
    }
    //! [Create the importer of Caffe model]

    if (!importer)
    {
        cerr << "Can't load network by using the following files: " << endl;
        cerr << "prototxt:   " << modelConfiguration << endl;
        cerr << "caffemodel: " << modelBinary << endl;
        cerr << "Models can be downloaded here:" << endl;
        cerr << "https://github.com/weiliu89/caffe/tree/ssd#models" << endl;
        exit(-1);
    }

    //! [Initialize network]
    dnn::Net net;
    importer->populateNet(net);
    importer.release();          //We don't need importer anymore
    //! [Initialize network]

    cv::Mat frame = cv::imread(parser.get<string>("image"), -1);

    //! [Prepare blob]
    Mat preprocessedFrame = preprocess(frame);

    Mat inputBlob = blobFromImage(preprocessedFrame); //Convert Mat to batch of images
    //! [Prepare blob]

    //! [Set input blob]
    net.setBlob(".data", inputBlob);                //set the network input
    //! [Set input blob]

    //! [Make forward pass]
    net.forward();                                  //compute output
    //! [Make forward pass]

    //! [Gather output]
    Mat detection = net.getBlob("detection_out");
    Mat detectionMat(detection.size[2], detection.size[3], CV_32F, detection.ptr<float>());

    float confidenceThreshold = parser.get<float>("min_confidence");
    for(int i = 0; i < detectionMat.rows; i++)
    {
        float confidence = detectionMat.at<float>(i, 2);

        if(confidence > confidenceThreshold)
        {
            size_t objectClass = detectionMat.at<float>(i, 1);

            float xLeftBottom = detectionMat.at<float>(i, 3) * frame.cols;
            float yLeftBottom = detectionMat.at<float>(i, 4) * frame.rows;
            float xRightTop = detectionMat.at<float>(i, 5) * frame.cols;
            float yRightTop = detectionMat.at<float>(i, 6) * frame.rows;

            std::cout << "Class: " << objectClass << std::endl;
            std::cout << "Confidence: " << confidence << std::endl;

            std::cout << " " << xLeftBottom
                      << " " << yLeftBottom
                      << " " << xRightTop
                      << " " << yRightTop << std::endl;

            Rect object(xLeftBottom, yLeftBottom,
                        xRightTop - xLeftBottom,
                        yRightTop - yLeftBottom);

            rectangle(frame, object, Scalar(0, 255, 0));
        }
    }

    imshow("detections", frame);
    waitKey();

    return 0;
} // main
