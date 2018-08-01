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
#include <opencv2/core/ocl.hpp>
#include "opencl_kernels_dnn.hpp"
#include "op_im2col.hpp"
#include "opencl_kernels_dnn.hpp"

namespace cv {
namespace dnn {

#if 0
template <typename Dtype>
class im2col_CpuPBody : public cv::ParallelLoopBody
{
    const Dtype* data_im;
    int channels, height, width;
    int kernel_h, kernel_w;
    int pad_h, pad_w;
    int stride_h, stride_w;
    int dilation_h, dilation_w;
    Dtype* data_col;
    int height_col, width_col, channels_col;

    im2col_CpuPBody() {}
public:

    static void run(const Dtype* data_im,
                    int channels, int height, int width,
                    int kernel_h, int kernel_w,
                    int pad_h, int pad_w,
                    int stride_h, int stride_w,
                    int dilation_h, int dilation_w,
                    int height_col, int width_col,
                    Dtype* data_col)
    {
        im2col_CpuPBody<Dtype> t;

        t.data_im = data_im;
        t.data_col = data_col;
        t.channels = channels; t.height = height; t.width = width;
        t.kernel_h = kernel_h; t.kernel_w = kernel_w;
        t.pad_h = pad_h; t.pad_w = pad_w;
        t.stride_h = stride_h; t.stride_w = stride_w;
        t.dilation_h = dilation_h; t.dilation_w = dilation_w;

        t.height_col = height_col;
        t.width_col = width_col;
        t.channels_col = channels * kernel_h * kernel_w;

        cv::parallel_for_(Range(0, t.channels_col), t);
    }

    virtual void operator ()(const Range &r) const
    {
        for (int c = r.start; c < r.end; ++c)
        {
            int w_offset = c % kernel_w;
            int h_offset = (c / kernel_w) % kernel_h;
            int c_im = c / kernel_h / kernel_w;
            for (int h = 0; h < height_col; ++h)
            {
                for (int w = 0; w < width_col; ++w)
                {
                    int h_pad = h * stride_h - pad_h + h_offset * dilation_h;
                    int w_pad = w * stride_w - pad_w + w_offset * dilation_w;
                    if (h_pad >= 0 && h_pad < height && w_pad >= 0 && w_pad < width)
                        data_col[(c * height_col + h) * width_col + w] =
                        data_im[(c_im * height + h_pad) * width + w_pad];
                    else
                        data_col[(c * height_col + h) * width_col + w] = 0;
                }
            }
        }
    }
};
#endif

template <typename Dtype>
class im2row_CpuPBody : public cv::ParallelLoopBody
{
    const Dtype* data_im;
    int channels, height, width;
    int kernel_h, kernel_w;
    int pad_h, pad_w;
    int stride_h, stride_w;
    int dilation_h, dilation_w;
    Dtype* data_col;
    int height_col, width_col, channels_col;

    im2row_CpuPBody() {}
public:

    static void run(const Dtype* data_im,
                    int channels, int height, int width,
                    int kernel_h, int kernel_w,
                    int pad_h, int pad_w,
                    int stride_h, int stride_w,
                    int dilation_h, int dilation_w,
                    int height_col, int width_col,
                    Dtype* data_col)
    {
        im2row_CpuPBody<Dtype> t;

        t.data_im = data_im;
        t.data_col = data_col;
        t.channels = channels; t.height = height; t.width = width;
        t.kernel_h = kernel_h; t.kernel_w = kernel_w;
        t.pad_h = pad_h; t.pad_w = pad_w;
        t.stride_h = stride_h; t.stride_w = stride_w;
        t.dilation_h = dilation_h; t.dilation_w = dilation_w;

        t.height_col = height_col;
        t.width_col = width_col;
        t.channels_col = channels * kernel_h * kernel_w;

        int total = t.height_col*t.width_col;
#if 1
        t(Range(0, total));
#else
        cv::parallel_for_(Range(0, total), t, 16);
#endif
    }

    virtual void operator ()(const Range &r) const
    {
        int dh = dilation_h, dw = dilation_w;
        int kh = kernel_h, kw = kernel_w;
        Dtype* data_col_ = data_col;
        const Dtype* data_im_ = data_im;
        int kelems = kh*kw;
        AutoBuffer<int> ofs_(kelems);
        int* ofs = ofs_;
        int k = 0;
        for( int k_r = 0; k_r < kernel_h; k_r++ )
            for( int k_c = 0; k_c < kernel_w; k_c++, k++ )
                ofs[k] = k_r*dh*width + k_c*dw;

        for (int row = r.start; row < r.end; ++row)
        {
            int out_c = row % width_col;
            int out_r = row / width_col;
            int out_row_offset = row*kh*kw*channels;

            int start_in_r = out_r * stride_h - pad_h;
            int start_in_c = out_c * stride_w - pad_w;
            int start_k_r = std::max(0, (-start_in_r + dilation_h-1)/dilation_h);
            int end_k_r = std::min(kh, (height - start_in_r + dilation_h-1)/dilation_h);
            int start_k_c = std::max(0, (-start_in_c + dilation_w-1)/dilation_w);
            int end_k_c = std::min(kw, (width - start_in_c + dilation_w-1)/dilation_w);

            if( start_k_r == 0 && end_k_r == kh && start_k_c == 0 && end_k_c == kw )
            {
                for( int i_c = 0; i_c < channels; i_c++ )
                {
                    float* data_col_c = data_col_ + out_row_offset + i_c*kh*kw;
                    const float* data_im_c = data_im_ + (i_c*height + start_in_r)*width + start_in_c;

                    for( k = 0; k < kelems; k++ )
                    {
                        data_col_c[k] = data_im_c[ofs[k]];
                    }
                }
            }
            else
            {
                memset(data_col_, 0, kw*kh*channels*sizeof(data_col_[0]));
                for(int i_c = 0; i_c < channels; i_c++)
                {
                    int channels_offset = i_c * width * height;
                    int out_ch_offset = i_c*kh*kw;
                    int in_r = start_in_r + start_k_r*dh;

                    for(int k_r = start_k_r; k_r < end_k_r; k_r++, in_r += dh)
                    {
                        int row_offset = in_r*width;
                        int out_col_offset = k_r*kw;
                        int in_c = start_in_c + start_k_c*dw;

                        for(int k_c = start_k_c; k_c < end_k_c; k_c++, in_c += dw)
                        {
                            int in_index = channels_offset + row_offset + in_c;
                            int out_index = out_row_offset + out_ch_offset + out_col_offset + k_c;

                            data_col_[out_index] = data_im_[in_index];
                        }
                    }
                }
            }
        }
    }
};

void im2row(const float* data_im, int channels, int height, int width,
            int kernel_h, int kernel_w, int pad_h, int pad_w,
            int stride_h, int stride_w, int dilation_h, int dilation_w,
            int height_col, int width_col, float* data_col)
{
    im2row_CpuPBody<float>::run(data_im, channels, height, width,
                                kernel_h, kernel_w, pad_h, pad_w,
                                stride_h, stride_w, dilation_h, dilation_w,
                                height_col, width_col, data_col);
}


#if 0
template <typename Dtype>
class col2im_CpuPBody : public cv::ParallelLoopBody
{
    const Dtype* data_col;
    int channels, height, width;
    int kernel_h, kernel_w;
    int pad_h, pad_w;
    int stride_h, stride_w;
    Dtype* data_im;
    int height_col, width_col;

    col2im_CpuPBody() {}

public:
    static void run(const Dtype* data_col,
                    int channels, int height, int width,
                    int kernel_h, int kernel_w,
                    int pad_h, int pad_w,
                    int stride_h, int stride_w,
                    Dtype* data_im)
    {
        //TODO: single-threaded version switch

        col2im_CpuPBody t;
        t.data_col = data_col;
        t.data_im = data_im;
        t.channels = channels; t.height = height; t.width = width;
        t.kernel_h = kernel_h; t.kernel_w = kernel_w;
        t.pad_h = pad_h; t.pad_w = pad_w;
        t.stride_h = stride_h; t.stride_w = stride_w;
        t.height_col = (height + 2 * pad_h - kernel_h) / stride_h + 1;
        t.width_col = (width + 2 * pad_w - kernel_w) / stride_w + 1;
        int img_total = channels * height * width;

        cv::parallel_for_(Range(0, img_total), t);
    }

    virtual void operator ()(const Range &r) const
    {
        const Dtype* data_col_ = data_col;
        Dtype* data_im_ = data_im;
        int coeff_h_col = (1 - stride_h * kernel_w * height_col) * width_col;
        int coeff_w_col = (1 - stride_w * height_col * width_col);
        for (int index = r.start; index < r.end; index++)
        {
            Dtype val = 0;
            int w = index % width + pad_w;
            int h = (index / width) % height + pad_h;
            int c = index / (width * height);

            // compute the start and end of the output
            int w_col_start = (w < kernel_w) ? 0 : (w - kernel_w) / stride_w + 1;
            int w_col_end = std::min(w / stride_w + 1, width_col);
            int h_col_start = (h < kernel_h) ? 0 : (h - kernel_h) / stride_h + 1;
            int h_col_end = std::min(h / stride_h + 1, height_col);

            // equivalent implementation
            int offset =
            (c * kernel_h * kernel_w + h * kernel_w + w) * height_col * width_col;

            for (int h_col = h_col_start; h_col < h_col_end; ++h_col) {
                for (int w_col = w_col_start; w_col < w_col_end; ++w_col) {
                    val += data_col_[offset + h_col * coeff_h_col + w_col * coeff_w_col];
                }
            }
            data_im_[index] = val;
        }
    }
};
#endif

//single-threaded version
template <typename Dtype>
void col2im_cpu(const Dtype* data_col,
                int channels, int height, int width,
                int kernel_h, int kernel_w,
                int pad_h, int pad_w,
                int stride_h, int stride_w,
                int dilation_h, int dilation_w,
                Dtype* data_im,
                const int* ofsbuf)
{
    int height_col = (height + 2 * pad_h - (dilation_h * (kernel_h - 1) + 1)) / stride_h + 1;
    int width_col = (width + 2 * pad_w - (dilation_w * (kernel_w - 1) + 1)) / stride_w + 1;
    int channels_col = channels * kernel_h * kernel_w;

    std::memset(data_im, 0, height * width * channels * sizeof(Dtype));

    for (int c = 0; c < channels_col; ++c, ofsbuf += 3)
    {
        //int w_offset = c % kernel_w;
        //int h_offset = (c / kernel_w) % kernel_h;
        //int c_im = c / kernel_h / kernel_w;
        int w_offset = ofsbuf[0];
        int h_offset = ofsbuf[1];
        int c_im = ofsbuf[2];

        for (int h = 0; h < height_col; ++h)
        {
            for (int w = 0; w < width_col; ++w)
            {
                int h_pad = h * stride_h - pad_h + h_offset * dilation_h;
                int w_pad = w * stride_w - pad_w + w_offset * dilation_w;

                if (h_pad >= 0 && h_pad < height && w_pad >= 0 && w_pad < width)
                    data_im[(c_im * height + h_pad) * width + w_pad] +=
                    data_col[(c * height_col + h) * width_col + w];
            }
        }
    }
}

void col2im(const float* data_col, int channels, int height, int width,
            int kernel_h, int kernel_w, int pad_h, int pad_w,
            int stride_h, int stride_w, int dilation_h, int dilation_w,
            float* data_im, const int* ofsbuf)
{
    //col2im_CpuPBody<float>::run(data_col, channels, height, width, kernel_h, kernel_w, pad_h, pad_w, stride_h, stride_w, data_im);
    col2im_cpu(data_col, channels, height, width, kernel_h, kernel_w, pad_h, pad_w,
               stride_h, stride_w, dilation_h, dilation_w, data_im, ofsbuf);
}

}
}
