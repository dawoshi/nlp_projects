# Onnxruntime deploy

## 介绍

1. 使用onnxruntime 对训练的模型进行c++部署

2. 使用bazel进行编译

## 环境

- bazel 2.0.0
- onnxruntime-linux-x64-1.13.1

## 效果

test courps len count: 10000

Totle run Time : 226222ms

title级别的推理速度达到 22ms

## 使用说明
```
# 编译：
bazel build //model:model_test 
# 执行
需要 model.onnx , vocab.txt 文件
nohup ./bazel-bin/model/model_test

```

## 参考
[1] https://github.com/lonePatient/BERT-NER-Pytorch