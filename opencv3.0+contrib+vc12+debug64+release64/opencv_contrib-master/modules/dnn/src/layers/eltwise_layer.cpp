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

class EltwiseLayerImpl : public EltwiseLayer
{
public:
    EltwiseOp op;
    std::vector<int> coeffs;

    EltwiseLayerImpl(const LayerParams& params)
    {
        setParamsFrom(params);
        op = EltwiseLayer::SUM;
        if (params.has("operation"))
        {
            String operation = params.get<String>("operation").toLowerCase();
            if (operation == "prod")
                op = EltwiseLayer::PROD;
            else if (operation == "sum")
                op = EltwiseLayer::SUM;
            else if (operation == "max")
                op = EltwiseLayer::MAX;
            else
                CV_Error(cv::Error::StsBadArg, "Unknown operaticon type \"" + operation + "\"");
        }

        if (params.has("coeff"))
        {
            DictValue paramCoeff = params.get("coeff");
            int i, n = paramCoeff.size();
            coeffs.resize(n);
            for (i = 0; i < n; i++)
            {
                coeffs[i] = paramCoeff.get<int>(i);
            }
        }
    }

    void allocate(const std::vector<Mat *> &inputs, std::vector<Mat> &outputs)
    {
        CV_Assert(2 <= inputs.size());
        CV_Assert(coeffs.size() == 0 || coeffs.size() == inputs.size());
        CV_Assert(op == SUM || coeffs.size() == 0);

        for (size_t i = 1; i < inputs.size(); ++i)
        {
            CV_Assert(inputs[i]->size == inputs[0]->size);
        }
        outputs.resize(1);
        outputs[0].create(inputs[0]->dims, inputs[0]->size.p, inputs[0]->type());
    }

    void forward(std::vector<Mat *> &inputs, std::vector<Mat> &outputs)
    {
        Mat& output = outputs[0];
        switch (op)
        {
            case SUM:
                CV_Assert(coeffs.size() == 0 || coeffs.size() == inputs.size());
                if (0 < coeffs.size())
                {
                    output.setTo(0.);
                    for (size_t i = 0; i < inputs.size(); i++)
                    {
                        output += *inputs[i] * coeffs[i];
                    }
                }
                else
                {
                    add(*inputs[0], *inputs[1], output);
                    for (size_t i = 2; i < inputs.size(); i++)
                    {
                        output += *inputs[i];
                    }
                }
                break;
            case PROD:
                output.setTo(1.);
                for (size_t i = 0; i < inputs.size(); i++)
                {
                    output = output.mul(*inputs[i]);
                }
                break;
            case MAX:
                cv::max(*inputs[0], *inputs[1], output);
                for (size_t i = 2; i < inputs.size(); i++)
                {
                    cv::max(output, *inputs[i], output);
                }
                break;
            default:
                CV_Assert(0);
                break;
        }
    }
};

Ptr<EltwiseLayer> EltwiseLayer::create(const LayerParams& params)
{
    return Ptr<EltwiseLayer>(new EltwiseLayerImpl(params));
}

}
}
