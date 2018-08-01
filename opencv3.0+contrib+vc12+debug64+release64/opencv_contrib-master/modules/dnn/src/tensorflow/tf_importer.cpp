// This file is part of OpenCV project.
// It is subject to the license terms in the LICENSE file found in the top-level directory
// of this distribution and at http://opencv.org/license.html.

// Copyright (C) 2016, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.

/*
Implementation of Tensorflow models parser
*/

#include "../precomp.hpp"
using namespace cv;
using namespace cv::dnn;

#if HAVE_PROTOBUF
#include "graph.pb.h"

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <google/protobuf/message.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>
#include "tf_io.hpp"

using ::google::protobuf::RepeatedField;
using ::google::protobuf::RepeatedPtrField;
using ::google::protobuf::Message;
using ::google::protobuf::Descriptor;
using ::google::protobuf::FieldDescriptor;
using ::google::protobuf::Reflection;

namespace
{

static int toNCHW[] = {0, 2, 3, 1};

typedef std::vector<std::pair<String, int> > StrIntVector;

struct Pin
{
    Pin(const std::string &_name, int _blobIndex = 0) :
        name(_name), blobIndex(_blobIndex) {}

    Pin() :
        name(""), blobIndex(-1) {}

    std::string name;
    int blobIndex;
};

void blobShapeFromTensor(const tensorflow::TensorProto &tensor, std::vector<int>& shape)
{
    shape.clear();
    if (tensor.has_tensor_shape())
    {
        const tensorflow::TensorShapeProto &_shape = tensor.tensor_shape();
        int i, n = _shape.dim_size();
        shape.resize(n);

        for (i = 0; i < n; i++)
            shape[i] = (int)_shape.dim(i).size();
    }
    else
    {
        CV_Error(Error::StsError, "Unknown shape of input tensor");
    }
}

template <typename T>
void parseTensor(const tensorflow::TensorProto &tensor, Mat &dstBlob)
{
    std::vector<int> shape;
    blobShapeFromTensor(tensor, shape);
    int dims = (int)shape.size();

    if (dims == 4)
    {
        // REORDER blob NHWC to NCHW
        swap(shape[2], shape[3]); // NHCW
        swap(shape[1], shape[2]); // NCHW
    }

    dstBlob.create(shape, CV_32F);

    int size = tensor.tensor_content().size() / sizeof(T);
    CV_Assert(size == (int)dstBlob.total());

    float *dstData = dstBlob.ptr<float>();
    const T *data = reinterpret_cast<const T*>(tensor.tensor_content().c_str());

    if (dims == 4)
    {
        int num = shape[0], channels = shape[1], height = shape[2], width = shape[3];
        int total = num*channels*height*width;
        for(int i_n = 0; i_n < shape[0]; i_n++) {
            for(int i_c = 0; i_c < shape[1]; i_c++) {
                for(int i_h = 0; i_h < shape[2]; i_h++) {
                    for(int i_w = 0; i_w < shape[3]; i_w++) {
                       int dst_i = channels*height*width*i_n + height*width*i_c + width*i_h + i_w;
                       int src_i = channels*height*width*i_n + i_c + channels*width*i_h + channels*i_w;

                       CV_Assert(dst_i < total);
                       CV_Assert(src_i < total);

                       dstData[dst_i] = data[src_i];
                    }
                }
            }
        }
    } else {
        for (int i = 0; i < size; i++)
            dstData[i] = data[i];
    }
}

void blobFromTensor(const tensorflow::TensorProto &tensor, Mat &dstBlob)
{
    switch (tensor.dtype()) {
        case tensorflow::DT_FLOAT:
            parseTensor<float>(tensor, dstBlob);
            break;
        case tensorflow::DT_DOUBLE:
            parseTensor<double>(tensor, dstBlob);
            break;
        default:
            CV_Error(Error::StsError, "Tensor's data type is not supported");
            break;
    }
}

void printList(const tensorflow::AttrValue::ListValue &val)
{
    std::cout << "(";
    for (int i = 0; i < val.i_size(); i++)
        std::cout << " " << val.i(i);
    std::cout << " )";
}

void printTensorShape(const tensorflow::TensorShapeProto &shape)
{
    std::cout << "[ ";
    for (int d = 0; d < shape.dim_size(); d++)
        std::cout << shape.dim(d).name() <<
                     ":" << shape.dim(d).size() << " ";
    std::cout << "]";
}

void printTensor(const tensorflow::TensorProto &tensor)
{
    printTensorShape(tensor.tensor_shape());

    if (tensor.tensor_content().empty())
        return;

    switch (tensor.dtype())
    {
    case 1:  // float
        {
            const float *data = reinterpret_cast<const float*>(tensor.tensor_content().c_str());
            int size = tensor.tensor_content().size() / sizeof(float);
            for (int i = 0; i < std::min(10, size); i++)
                std::cout << " " << data[i];
            if (size > 10)
                std::cout << " ... " << size - 10 << " more";
            break;
        }
    case 3:  // int32
        {
            const int *data = reinterpret_cast<const int*>(tensor.tensor_content().c_str());
            int size = tensor.tensor_content().size() / sizeof(int);
            for (int i = 0; i < std::min(10, size); i++)
                std::cout << " " << data[i];
            if (size > 10)
                std::cout << " ... " << size - 10 << " more";
            break;
        }
    default:
        CV_Error(Error::StsError, "Tensor type is not supported");
        break;
    }
}

void printLayerAttr(const tensorflow::NodeDef &layer)
{
    std::cout << std::endl << layer.name() << ":" << layer.op();
    for (int ii = 0; ii < layer.input_size(); ii++)
        std::cout << "(" << layer.input(ii) << ")";
    std::cout << std::endl;
    google::protobuf::Map<std::string, tensorflow::AttrValue> attr
            = layer.attr();
    for (google::protobuf::Map<std::string, tensorflow::AttrValue>::const_iterator ai = attr.begin();
         ai != attr.end(); ++ai)
    {
        std::cout << ai->first << ":";
        if (ai->first == "dtype" || ai->first == "T")
            std::cout << ai->second.i();
        else if (ai->first == "padding")
            std::cout << ai->second.s();
        else if (ai->first == "transpose_a" || ai->first == "transpose_b")
            std::cout << ai->second.b();
        //            else if (ai->first == "shape")
        //              printTensorShape(ai->second.shape());
        else if (ai->first == "strides" || ai->first == "ksize")
            printList(ai->second.list());
        else
            printTensor(ai->second.tensor());
        std::cout << std::endl;
    }
}

bool hasLayerAttr(const tensorflow::NodeDef &layer, const std::string &name)
{
    google::protobuf::Map<std::string, tensorflow::AttrValue> attr = layer.attr();
    return attr.find(name) != attr.end();
}

const tensorflow::AttrValue& getLayerAttr(const tensorflow::NodeDef &layer, const std::string &name)
{
    return layer.attr().at(name);
}

void setStrides(LayerParams &layerParams, const tensorflow::NodeDef &layer)
{
    if (hasLayerAttr(layer, "strides"))
    {
        const tensorflow::AttrValue& val = getLayerAttr(layer, "strides");
        if (val.list().i_size() != 4 ||
            val.list().i(0) != 1 || val.list().i(3) != 1)
            CV_Error(Error::StsError, "Unsupported strides");
        layerParams.set("stride_h", static_cast<int>(val.list().i(1)));
        layerParams.set("stride_w", static_cast<int>(val.list().i(2)));
    }
}

DictValue parseDims(const tensorflow::TensorProto &tensor) {
    std::vector<int> shape;
    blobShapeFromTensor(tensor, shape);
    int dims = (int)shape.size();

    CV_Assert(tensor.dtype() == tensorflow::DT_INT32);
    CV_Assert(dims == 1);

    int size = tensor.tensor_content().size() / sizeof(int);
    const int *data = reinterpret_cast<const int*>(tensor.tensor_content().c_str());
    // TODO: add reordering shape if dims == 4
    return DictValue::arrayInt(data, size);
}

void setKSize(LayerParams &layerParams, const tensorflow::NodeDef &layer)
{
    if (hasLayerAttr(layer, "ksize"))
    {
        const tensorflow::AttrValue& val = getLayerAttr(layer, "ksize");
        if (val.list().i_size() != 4 ||
            val.list().i(0) != 1 || val.list().i(3) != 1)
            CV_Error(Error::StsError, "Unsupported ksize");
        layerParams.set("kernel_h", static_cast<int>(val.list().i(1)));
        layerParams.set("kernel_w", static_cast<int>(val.list().i(2)));
    }
    else
    {
        layerParams.set("kernel_h", 1);
        layerParams.set("kernel_w", 1);
    }
}

void setPadding(LayerParams &layerParams, const tensorflow::NodeDef &layer)
{
    if (hasLayerAttr(layer, "padding"))
        layerParams.set("pad_mode", getLayerAttr(layer, "padding").s());
}

void RemoveIdentityOps(tensorflow::GraphDef& net) {
    typedef std::map<String, String>  IdentityOpsMap;
    IdentityOpsMap identity_ops;

    std::vector<int> identity_ops_idx;

    int layersCount = net.node_size();
    for (int li = 0; li < layersCount; li++)
    {
        const tensorflow::NodeDef &layer = net.node(li);
        String type = layer.op();

        if (type == "Identity") {
            identity_ops_idx.push_back(li);
            identity_ops[layer.name()] = layer.input(0);
        }
    }

    for (int li = 0; li < layersCount; li++)
    {
        tensorflow::NodeDef* layer = net.mutable_node(li);
        for (int input_id = 0; input_id < layer->input_size(); input_id++) {
            String input_op_name = layer->input(input_id);
            IdentityOpsMap::iterator it = identity_ops.find(input_op_name);

            if (it != identity_ops.end()) {
                layer->set_input(input_id, it->second);
            }
        }
    }

    std::sort(identity_ops_idx.begin(), identity_ops_idx.end());

    int removed_nodes = 0;
    for(size_t i = 0; i < identity_ops_idx.size(); i++) {
        int start_id = identity_ops_idx[i] - removed_nodes;
        net.mutable_node()->DeleteSubrange(start_id, 1);
        removed_nodes++;
    }
}

Pin parsePin(const std::string &name)
{
    Pin pin(name);

    size_t delimiter_pos = name.find_first_of(":");
    if (delimiter_pos != std::string::npos)
    {
        pin.name = name.substr(0, delimiter_pos);
        std::istringstream(name.substr(delimiter_pos + 1)) >> pin.blobIndex;
    }

    return pin;
}

StrIntVector getNextLayers(const tensorflow::GraphDef& net, const String& layer_name, const String& type = "")
{
   StrIntVector layers;

   for (int li = 0; li < net.node_size(); li++)
   {
       const tensorflow::NodeDef& layer = net.node(li);
       for (int input_id = 0; input_id < layer.input_size(); input_id++) {
           String input_op_name = parsePin(layer.input(input_id)).name;
           bool type_ok = type.empty() ? true : type == layer.op();
           if (input_op_name == layer_name && type_ok)
               layers.push_back(std::make_pair(layer.name(), li));
       }
   }

   return layers;
}

void ExcludeLayer(tensorflow::GraphDef& net, const int layer_index, const int input_blob_index, bool remove_from_net = true) {
    String layer_name = net.node(layer_index).name();
    StrIntVector layers = getNextLayers(net, layer_name);

    String removed_layer_input = net.node(layer_index).input(input_blob_index);

    for (size_t i = 0; i < layers.size(); i++)
    {
        tensorflow::NodeDef* layer = net.mutable_node(layers[i].second);
        for (int input_id = 0; input_id < layer->input_size(); input_id++) {
                String input_op_name = layer->input(input_id);

                if (input_op_name == layer_name) {
                    layer->set_input(input_id, removed_layer_input);
                }
        }
    }

    if (remove_from_net)
        net.mutable_node()->DeleteSubrange(layer_index, 1);
}

class TFImporter : public Importer {
public:
    TFImporter(const char *model);
    void populateNet(Net dstNet);
    ~TFImporter() {}

private:
    void kernelFromTensor(const tensorflow::TensorProto &tensor, Mat &dstBlob);

    void connect(const std::map<String, int>& layers_name_id_map, Net& network, const Pin& outPin,
                 const int input_layer_id, const int input_blob_id);
    void connectToAllBlobs(const std::map<String, int>& layer_id, Net& network, const Pin& outPin,
                           const int input_layer_id, const int input_blobs_count);
    const tensorflow::TensorProto& getConstBlob(const tensorflow::NodeDef &layer, std::map<String, int> const_layers,
                                                int input_blob_index = -1, int* actual_inp_blob_idx = 0);


    tensorflow::GraphDef net;
};

TFImporter::TFImporter(const char *model)
{
    if (model && model[0])
        ReadTFNetParamsFromBinaryFileOrDie(model, &net);
}

void TFImporter::kernelFromTensor(const tensorflow::TensorProto &tensor, Mat &dstBlob)
{
    std::vector<int> shape;
    blobShapeFromTensor(tensor, shape);
    int dims = (int)shape.size();

    // TODO: other blob types
    CV_Assert(tensor.dtype() == tensorflow::DT_FLOAT);
    CV_Assert(dims == 4);

    // REORDER kernel HWIO to OIHW
    swap(shape[0], shape[2]); // IWHO
    swap(shape[1], shape[3]); // IOHW
    swap(shape[0], shape[1]); // OIHW

    dstBlob.create(shape, CV_32F);

    int size = tensor.tensor_content().size() / sizeof(float);
    CV_Assert(size == (int)dstBlob.total());

    float *dstData = dstBlob.ptr<float>();
    const float *data = reinterpret_cast<const float*>(tensor.tensor_content().c_str());

    int out_c = shape[0], input_c = shape[1], height = shape[2], width = shape[3];
    int total = out_c*input_c*height*width;
    for(int i_oc = 0; i_oc < out_c; i_oc++) {
        for(int i_ic = 0; i_ic < input_c; i_ic++) {
            for(int i_h = 0; i_h < height; i_h++) {
                for(int i_w = 0; i_w < width; i_w++) {
                    int dst_i = input_c*height*width*i_oc + height*width*i_ic + width*i_h + i_w;
                    int src_i = out_c*input_c*width*i_h + out_c*input_c*i_w + out_c*i_ic + i_oc;
                    CV_Assert(dst_i < total);
                    CV_Assert(src_i < total);
                   dstData[dst_i] = data[src_i];
                }
            }
        }
    }
}

void TFImporter::connect(const std::map<String, int>& layers_name_id_map, Net& network, const Pin& outPin,
             const int input_layer_id, const int input_blob_id)
{
    std::map<String, int>::const_iterator it = layers_name_id_map.find(outPin.name);
    if (it == layers_name_id_map.end())
        CV_Error(Error::StsError, "Input layer not found: " + outPin.name);
    network.connect(it->second, outPin.blobIndex, input_layer_id, input_blob_id);
}

void TFImporter::connectToAllBlobs(const std::map<String, int>& layer_id, Net& network, const Pin& outPin,
                     const int input_layer_id, const int input_blobs_count)
{
    for (int input_blob_id = 0; input_blob_id < input_blobs_count; input_blob_id++)
        connect(layer_id, network, outPin, input_layer_id, input_blob_id);
}

const tensorflow::TensorProto& TFImporter::getConstBlob(const tensorflow::NodeDef &layer, std::map<String, int> const_layers,
                                              int input_blob_index, int* actual_inp_blob_idx) {
    if (input_blob_index == -1) {
        for(int i = 0; i < layer.input_size(); i++) {
            Pin input = parsePin(layer.input(i));
            if (const_layers.find(input.name) != const_layers.end()) {
                if (input_blob_index != -1)
                    CV_Error(Error::StsError, "More than one input is Const op");

                input_blob_index = i;
            }
        }
    }

    if (input_blob_index == -1)
        CV_Error(Error::StsError, "Const input blob for weights not found");

    Pin kernel_inp = parsePin(layer.input(input_blob_index));
    if (const_layers.find(kernel_inp.name) == const_layers.end())
        CV_Error(Error::StsError, "Const kernel input not found");
    if (kernel_inp.blobIndex != 0)
        CV_Error(Error::StsError, "Unsupported kernel input");

    if(actual_inp_blob_idx) {
        *actual_inp_blob_idx = input_blob_index;
    }

    return net.node(const_layers.at(kernel_inp.name)).attr().at("value").tensor();
}


void TFImporter::populateNet(Net dstNet)
{
    RemoveIdentityOps(net);

    std::map<int, String> layers_to_ignore;

    int layersSize = net.node_size();

    // find all Const layers for params
    std::map<String, int> value_id;
    for (int li = 0; li < layersSize; li++)
    {
        const tensorflow::NodeDef &layer = net.node(li);
        String name = layer.name();
        String type = layer.op();

        if (type != "Const")
            continue;  // only Const parameters are supported

        if (layer.attr().find("value") != layer.attr().end())
        {
            value_id.insert(std::make_pair(name, li));
        }

        layers_to_ignore[li] = name;
    }

    std::map<String, int> layer_id;

    for (int li = 0; li < layersSize; li++)
    {
        const tensorflow::NodeDef &layer = net.node(li);
        String name = layer.name();
        String type = layer.op();
        LayerParams layerParams;

        if(layers_to_ignore.find(li) != layers_to_ignore.end())
            continue;

        if (type == "Conv2D")
        {
            layerParams.set("bias_term", false);
            layerParams.blobs.resize(1);

            StrIntVector next_layers = getNextLayers(net, name, "BiasAdd");
            if (next_layers.size() == 1) {
                layerParams.set("bias_term", true);
                layerParams.blobs.resize(2);

                int weights_layer_index = next_layers[0].second;

                blobFromTensor(getConstBlob(net.node(weights_layer_index), value_id), layerParams.blobs[1]);
                ExcludeLayer(net, weights_layer_index, 0, false);
                layers_to_ignore[weights_layer_index] = next_layers[0].first;
            }

            kernelFromTensor(getConstBlob(layer, value_id), layerParams.blobs[0]);
            const int* kshape = layerParams.blobs[0].size.p;
            layerParams.set("kernel_h", kshape[2]);
            layerParams.set("kernel_w", kshape[3]);
            layerParams.set("num_output", kshape[0]);

            setStrides(layerParams, layer);
            setPadding(layerParams, layer);

            int id = dstNet.addLayer(name, "Convolution", layerParams);
            layer_id[name] = id;

            // one input only
            connect(layer_id, dstNet, parsePin(layer.input(0)), id, 0);
        }
        else if (type == "BiasAdd" || type == "Add")
        {
            layerParams.blobs.resize(1);
            blobFromTensor(getConstBlob(layer, value_id), layerParams.blobs[0]);

            int id = dstNet.addLayer(name, "Shift", layerParams);
            layer_id[name] = id;

            // one input only
            connect(layer_id, dstNet, parsePin(layer.input(0)), id, 0);
        }
        else if (type == "Identity")
        {
            int id = dstNet.addLayer(name, "Identity", layerParams);
            layer_id[name] = id;

            connectToAllBlobs(layer_id, dstNet, parsePin(layer.input(0)), id, layer.input_size());
        }
        else if (type == "MatMul")
        {
            CV_Assert(layer.input_size() == 2);

            layerParams.set("axis", 0);
            layerParams.set("bias_term", false);
            layerParams.blobs.resize(1);

            StrIntVector next_layers = getNextLayers(net, name, "BiasAdd");
            if (next_layers.size() == 1) {
                layerParams.set("bias_term", true);
                layerParams.blobs.resize(2);

                int weights_layer_index = next_layers[0].second;
                blobFromTensor(getConstBlob(net.node(weights_layer_index), value_id), layerParams.blobs[1]);
                ExcludeLayer(net, weights_layer_index, 0, false);
                layers_to_ignore[weights_layer_index] = next_layers[0].first;
            }

            int kernel_blob_index = -1;
            blobFromTensor(getConstBlob(layer, value_id, -1, &kernel_blob_index), layerParams.blobs[0]);

            if (kernel_blob_index == 1) { // In this case output is computed by x*W formula - W should be transposed
                Mat data = layerParams.blobs[0].t();
                layerParams.blobs[0] = data.clone();
            }

            layerParams.set("num_output", layerParams.blobs[0].size[0]);

            int id = dstNet.addLayer(name, "InnerProduct", layerParams);
            layer_id[name] = id;

            // one input only
            int input_blob_index = kernel_blob_index == 0 ? 1 : 0;
            connect(layer_id, dstNet, parsePin(layer.input(input_blob_index)), id, 0);
        }
        else if (type == "Reshape")
        {
            layerParams.set("dim", parseDims(getConstBlob(layer, value_id, 1)));
            layerParams.set("reorder_dims", true);

            int id = dstNet.addLayer(name, "Reshape", layerParams);
            layer_id[name] = id;

            // one input only
            connect(layer_id, dstNet, parsePin(layer.input(0)), id, 0);
        }
        else if (type == "Const")
        {
        }
        else if (type == "Softmax")
        {
            layerParams.set("axis", -1);
            int id = dstNet.addLayer(name, "Softmax", layerParams);
            layer_id[name] = id;

            connectToAllBlobs(layer_id, dstNet, parsePin(layer.input(0)), id, layer.input_size());
        }
        else if (type == "LRN")
        {
            if(hasLayerAttr(layer, "alpha")) {
                layerParams.set("alpha", getLayerAttr(layer, "alpha").f());
            }
            if(hasLayerAttr(layer, "beta")) {
                layerParams.set("beta", getLayerAttr(layer, "beta").f());
            }
            if(hasLayerAttr(layer, "depth_radius")) {
                int radius = (int)getLayerAttr(layer, "depth_radius").i();
                layerParams.set("local_size", 2*radius + 1);
            }
            if(hasLayerAttr(layer, "bias")) {
                layerParams.set("bias", getLayerAttr(layer, "bias").f());
            }
            layerParams.set("norm_by_size", false);

            int id = dstNet.addLayer(name, "LRN", layerParams);
            layer_id[name] = id;

            connectToAllBlobs(layer_id, dstNet, parsePin(layer.input(0)), id, layer.input_size());
        }
        else if (type == "Concat")
        {
            int axis = getConstBlob(layer, value_id, 0).int_val().Get(0);
            layerParams.set("axis", toNCHW[axis]);

            int id = dstNet.addLayer(name, "Concat", layerParams);
            layer_id[name] = id;

            // input(0) is concat_dim
            for (int ii = 1; ii < layer.input_size(); ii++)
            {
                Pin inp = parsePin(layer.input(ii));
                if (layer_id.find(inp.name) == layer_id.end())
                    CV_Error(Error::StsError, "Input layer not found: " + inp.name);
                dstNet.connect(layer_id.at(inp.name), inp.blobIndex, id, ii - 1);
            }
        }
        else if (type == "Relu")
        {
            int id = dstNet.addLayer(name, "ReLU", layerParams);
            layer_id[name] = id;

            connectToAllBlobs(layer_id, dstNet, parsePin(layer.input(0)), id, layer.input_size());
        }
        else if (type == "MaxPool")
        {
            layerParams.set("pool", "max");

            setKSize(layerParams, layer);
            setStrides(layerParams, layer);
            setPadding(layerParams, layer);

            int id = dstNet.addLayer(name, "Pooling", layerParams);
            layer_id[name] = id;

            connectToAllBlobs(layer_id, dstNet, parsePin(layer.input(0)), id, layer.input_size());
        }
        else if (type == "AvgPool")
        {
            layerParams.set("pool", "ave");

            setKSize(layerParams, layer);
            setStrides(layerParams, layer);
            setPadding(layerParams, layer);

            int id = dstNet.addLayer(name, "Pooling", layerParams);
            layer_id[name] = id;

            connectToAllBlobs(layer_id, dstNet, parsePin(layer.input(0)), id, layer.input_size());
        }
        else if (type == "Placeholder")
        {
            std::vector<String> netInputs(1);
            netInputs[0] = name;
            layer_id[name] = 0;
            dstNet.setNetInputs(netInputs);
        }
        else if (type == "Split") {
            // TODO: determing axis index remapping by input dimensions order of input blob
            // TODO: slicing input may be Const op
            // TODO: slicing kernels for convolutions - in current implenmentation it is impossible
            // TODO: add parsing num of slices parameter
            CV_Assert(layer.input_size() == 2);
            // num_split
            // 1st blob is dims tensor
            layerParams.set("slice_point", DictValue::arrayReal((double*)0, 0));

            int axis = getConstBlob(layer, value_id, 0).int_val().Get(0);
            layerParams.set("axis", toNCHW[axis]);

            int id = dstNet.addLayer(name, "Slice", layerParams);
            layer_id[name] = id;

            // one input only
            connect(layer_id, dstNet, parsePin(layer.input(1)), id, 0);
        }
        else
        {
            printLayerAttr(layer);
            CV_Error_(Error::StsError, ("Unknown layer type %s in op %s", type.c_str(), name.c_str()));
        }
    }
}

} // namespace

Net cv::dnn::readNetFromTensorflow(const String &model)
{
    Ptr<Importer> importer;
    try
    {
        importer = createTensorflowImporter(model);
    }
    catch(...)
    {
    }

    Net net;
    if (importer)
        importer->populateNet(net);
    return net;
}

Ptr<Importer> cv::dnn::createTensorflowImporter(const String &model)
{
    return Ptr<Importer>(new TFImporter(model.c_str()));
}

#else //HAVE_PROTOBUF

Ptr<Importer> cv::dnn::createTensorflowImporter(const String&)
{
    CV_Error(cv::Error::StsNotImplemented, "libprotobuf required to import data from TensorFlow models");
    return Ptr<Importer>();
}

#endif //HAVE_PROTOBUF
