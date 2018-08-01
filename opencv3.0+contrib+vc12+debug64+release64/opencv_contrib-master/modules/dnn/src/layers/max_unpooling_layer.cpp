// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

// Copyright (C) 2016, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.

/*
Implementation of Batch Normalization layer.
*/

#include "../precomp.hpp"
#include "layers_common.hpp"
#include <opencv2/dnn/shape_utils.hpp>

namespace cv
{
namespace dnn
{

class MaxUnpoolLayerImpl : public MaxUnpoolLayer
{
public:
    MaxUnpoolLayerImpl(const LayerParams& params)
    {
        setParamsFrom(params);
        poolKernel = Size(params.get<int>("pool_k_w"), params.get<int>("pool_k_h"));
        poolPad = Size(params.get<int>("pool_pad_w"), params.get<int>("pool_pad_h"));
        poolStride = Size(params.get<int>("pool_stride_w"), params.get<int>("pool_stride_h"));
    }

    void allocate(const std::vector<Mat*> &inputs, std::vector<Mat> &outputs)
    {
        CV_Assert(inputs.size() == 2);
        const Mat& inp0 = *inputs[0];
        CV_Assert(inp0.total() == inputs[1]->total());
        CV_Assert(inp0.dims == 4);

        int outShape[] = { inp0.size[0], inp0.size[1], inp0.size[2], inp0.size[3] };
        outShape[2] = (outShape[2] - 1) * poolStride.height + poolKernel.height - 2 * poolPad.height;
        outShape[3] = (outShape[3] - 1) * poolStride.width + poolKernel.width - 2 * poolPad.width;

        outputs.resize(1);
        outputs[0].create(4, outShape, inp0.type());
    }

    void forward(std::vector<Mat*> &inputs, std::vector<Mat> &outputs)
    {
        CV_Assert(inputs.size() == 2);
        Mat& input = *inputs[0];
        Mat& indices = *inputs[1];

        CV_Assert(input.total() == indices.total());
        CV_Assert(input.size[0] == 1);
        CV_Assert(input.isContinuous());

        for(int i_n = 0; i_n < outputs.size(); i_n++)
        {
            Mat& outBlob = outputs[i_n];
            outBlob.setTo(0);
            CV_Assert(input.size[1] == outBlob.size[1]);
            int outPlaneTotal = outBlob.size[2]*outBlob.size[3];

            for (int i_c = 0; i_c < input.size[1]; i_c++)
            {
                Mat outPlane = getPlane(outBlob, 0, i_c);
                int wh_area = input.size[2]*input.size[3];
                const float* inptr = input.ptr<float>(0, i_c);
                const float* idxptr = indices.ptr<float>(0, i_c);
                float* outptr = outPlane.ptr<float>();

                for(int i_wh = 0; i_wh < wh_area; i_wh++)
                {
                    int index = idxptr[i_wh];
                    CV_Assert(0 <= index && index < outPlaneTotal);
                    outptr[index] = inptr[i_wh];
                }
            }
        }
    }

    Size poolKernel;
    Size poolPad;
    Size poolStride;
};

Ptr<MaxUnpoolLayer> MaxUnpoolLayer::create(const LayerParams& params)
{
    return Ptr<MaxUnpoolLayer>(new MaxUnpoolLayerImpl(params));
}

}
}
