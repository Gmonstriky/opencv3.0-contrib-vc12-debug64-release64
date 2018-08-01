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

#ifndef __OPENCV_DNN_DNN_SHAPE_UTILS_HPP__
#define __OPENCV_DNN_DNN_SHAPE_UTILS_HPP__

#include <opencv2/core.hpp>
#include <opencv2/core/types_c.h>
#include <ostream>

namespace cv {
namespace dnn {

//Useful shortcut
inline std::ostream &operator<< (std::ostream &s, cv::Range &r)
{
    return s << "[" << r.start << ", " << r.end << ")";
}

//Reshaping
//TODO: add -1 specifier for automatic size inferring

/*template<typename Mat>
void reshape(Mat &m, const BlobShape &shape)
{
    m = m.reshape(1, shape.dims(), shape.ptr());
}

template<typename Mat>
Mat reshaped(const Mat &m, const BlobShape &shape)
{
    return m.reshape(1, shape.dims(), shape.ptr());
}*/


//Slicing

struct _Range : public cv::Range
{
    _Range(const Range &r) : cv::Range(r) {}
    _Range(int start, int size = 1) : cv::Range(start, start + size) {}
};

static inline Mat slice(const Mat &m, const _Range &r0)
{
    Range ranges[CV_MAX_DIM];
    for (int i = 1; i < m.dims; i++)
        ranges[i] = Range::all();
    ranges[0] = r0;
    return m(&ranges[0]);
}

static inline Mat slice(const Mat &m, const _Range &r0, const _Range &r1)
{
    CV_Assert(m.dims >= 2);
    Range ranges[CV_MAX_DIM];
    for (int i = 2; i < m.dims; i++)
        ranges[i] = Range::all();
    ranges[0] = r0;
    ranges[1] = r1;
    return m(&ranges[0]);
}

static inline Mat slice(const Mat &m, const _Range &r0, const _Range &r1, const _Range &r2)
{
    CV_Assert(m.dims >= 3);
    Range ranges[CV_MAX_DIM];
    for (int i = 3; i < m.dims; i++)
        ranges[i] = Range::all();
    ranges[0] = r0;
    ranges[1] = r1;
    ranges[2] = r2;
    return m(&ranges[0]);
}

static inline Mat slice(const Mat &m, const _Range &r0, const _Range &r1, const _Range &r2, const _Range &r3)
{
    CV_Assert(m.dims >= 4);
    Range ranges[CV_MAX_DIM];
    for (int i = 4; i < m.dims; i++)
        ranges[i] = Range::all();
    ranges[0] = r0;
    ranges[1] = r1;
    ranges[2] = r2;
    ranges[3] = r3;
    return m(&ranges[0]);
}

static inline Mat getPlane(const Mat &m, int n, int cn)
{
    CV_Assert(m.dims > 2);
    Range range[CV_MAX_DIM];
    int sz[CV_MAX_DIM];
    for(int i = 2; i < m.dims; i++)
    {
        sz[i-2] = m.size.p[i];
        range[i] = Range::all();
    }
    range[0] = Range(n, n+1);
    range[1] = Range(cn, cn+1);
    return m(range).reshape(1, m.dims-2, sz);
}

static inline size_t shapeTotal(const std::vector<int>& shape)
{
    size_t i, n = shape.size(), p = 1;
    for( i = 0; i < n; i++ ) p *= shape[i];

    return p;
}

}
}
#endif
