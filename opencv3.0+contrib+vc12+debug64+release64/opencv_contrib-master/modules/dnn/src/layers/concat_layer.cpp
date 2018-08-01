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

#include "../precomp.hpp"
#include "layers_common.hpp"

namespace cv
{
namespace dnn
{

class ConcatLayerImpl : public ConcatLayer
{
public:
    ConcatLayerImpl(const LayerParams& params)
    {
        setParamsFrom(params);
        axis = params.get<int>("axis", 1);
    }

    void allocate(const std::vector<Mat *> &inputs, std::vector<Mat> &outputs)
    {
        CV_Assert(inputs.size() > 0);

        int dims = inputs[0]->dims, dtype = inputs[0]->type();
        std::vector<int> refShape(inputs[0]->size.p, inputs[0]->size.p + dims);
        axisIdx = axis < 0 ? axis + dims : axis;

        int axisSum = 0;
        for (size_t i = 0; i < inputs.size(); i++)
        {
            CV_Assert(inputs[i]->type() == dtype);
            for (int curAxis = 0; curAxis < dims; curAxis++)
            {
                if (curAxis != axisIdx && inputs[0]->size[curAxis] != inputs[i]->size[curAxis])
                    CV_Error(Error::StsBadSize, "Inconsitent shape for ConcatLayer");
            }

            axisSum += inputs[i]->size[axisIdx];
        }

        refShape[axisIdx] = axisSum;

        outputs.resize(1);
        outputs[0].create(dims, &refShape[0], dtype);
    }


    void forward(std::vector<Mat*> &inputs, std::vector<Mat> &outputs)
    {
        Mat& outMat = outputs[0];
        std::vector<Range> ranges(outputs[0].dims, Range::all());

        ranges[axisIdx].start = 0;
        for (size_t i = 0; i < inputs.size(); i++)
        {
            ranges[axisIdx].end = ranges[axisIdx].start + inputs[i]->size[axisIdx];
            inputs[i]->copyTo(outMat(&ranges[0]));
            ranges[axisIdx].start = ranges[axisIdx].end;
        }
    }

    int axisIdx;
};

Ptr<ConcatLayer> ConcatLayer::create(const LayerParams& params)
{
    return Ptr<ConcatLayer>(new ConcatLayerImpl(params));
}

}
}
