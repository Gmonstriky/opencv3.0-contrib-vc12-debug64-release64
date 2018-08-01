#include <opencv2/dnn.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
using namespace cv;
using namespace cv::dnn;

#include <fstream>
#include <iostream>
#include <cstdlib>
using namespace std;

static const string fcnType = "fcn8s";

static vector<cv::Vec3b> readColors(const string &filename = "pascal-classes.txt")
{
    vector<cv::Vec3b> colors;

    ifstream fp(filename.c_str());
    if (!fp.is_open())
    {
        cerr << "File with colors not found: " << filename << endl;
        exit(-1);
    }

    string line;
    while (!fp.eof())
    {
        getline(fp, line);
        if (line.length())
        {
            stringstream ss(line);

            string name; ss >> name;
            int temp;
            cv::Vec3b color;
            ss >> temp; color[0] = temp;
            ss >> temp; color[1] = temp;
            ss >> temp; color[2] = temp;
            colors.push_back(color);
        }
    }

    fp.close();
    return colors;
}

static void colorizeSegmentation(const Mat &score, const vector<cv::Vec3b> &colors, cv::Mat &segm)
{
    const int rows = score.size[2];
    const int cols = score.size[3];
    const int chns = score.size[1];

    cv::Mat maxCl(rows, cols, CV_8UC1);
    cv::Mat maxVal(rows, cols, CV_32FC1);
    for (int ch = 0; ch < chns; ch++)
    {
        for (int row = 0; row < rows; row++)
        {
            const float *ptrScore = score.ptr<float>(0, ch, row);
            uchar *ptrMaxCl = maxCl.ptr<uchar>(row);
            float *ptrMaxVal = maxVal.ptr<float>(row);
            for (int col = 0; col < cols; col++)
            {
                if (ptrScore[col] > ptrMaxVal[col])
                {
                    ptrMaxVal[col] = ptrScore[col];
                    ptrMaxCl[col] = ch;
                }
            }
        }
    }

    segm.create(rows, cols, CV_8UC3);
    for (int row = 0; row < rows; row++)
    {
        const uchar *ptrMaxCl = maxCl.ptr<uchar>(row);
        cv::Vec3b *ptrSegm = segm.ptr<cv::Vec3b>(row);
        for (int col = 0; col < cols; col++)
        {
            ptrSegm[col] = colors[ptrMaxCl[col]];
        }
    }

}

int main(int argc, char **argv)
{
    cv::dnn::initModule();          //Required if OpenCV is built as static libs

    String modelTxt = fcnType + "-heavy-pascal.prototxt";
    String modelBin = fcnType + "-heavy-pascal.caffemodel";
    String imageFile = (argc > 1) ? argv[1] : "rgb.jpg";

    vector<cv::Vec3b> colors = readColors();

    //! [Create the importer of Caffe model]
    Ptr<dnn::Importer> importer;
    try                                     //Try to import Caffe GoogleNet model
    {
        importer = dnn::createCaffeImporter(modelTxt, modelBin);
    }
    catch (const cv::Exception &err)        //Importer can throw errors, we will catch them
    {
        cerr << err.msg << endl;
    }
    //! [Create the importer of Caffe model]

    if (!importer)
    {
        cerr << "Can't load network by using the following files: " << endl;
        cerr << "prototxt:   " << modelTxt << endl;
        cerr << "caffemodel: " << modelBin << endl;
        cerr << fcnType << "-heavy-pascal.caffemodel can be downloaded here:" << endl;
        cerr << "http://dl.caffe.berkeleyvision.org/" << fcnType << "-heavy-pascal.caffemodel" << endl;
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
        cerr << "Can't read image from the file: " << imageFile << endl;
        exit(-1);
    }

    resize(img, img, Size(500, 500));       //FCN accepts 500x500 RGB-images
    Mat inputBlob = blobFromImage(img);   //Convert Mat to batch of images
    //! [Prepare blob]

    //! [Set input blob]
    net.setBlob(".data", inputBlob);        //set the network input
    //! [Set input blob]

    //! [Make forward pass]
    double t = (double)cv::getTickCount();
    net.forward();                          //compute output
    t = (double)cv::getTickCount() - t;
    printf("processing time: %.1fms\n", t*1000./getTickFrequency());
    //! [Make forward pass]

    //! [Gather output]
    Mat score = net.getBlob("score");

    Mat colorize;
    colorizeSegmentation(score, colors, colorize);
    Mat show;
    addWeighted(img, 0.4, colorize, 0.6, 0.0, show);
    imshow("show", show);
    waitKey(0);
    return 0;
} //main
