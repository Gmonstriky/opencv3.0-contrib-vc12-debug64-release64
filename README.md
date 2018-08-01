# opencv3.0-contrib-vc12-debug64-release64
opencv3.0+contrib+vc12+debug64+release64
opencv3.0加上contrib额外库进行cmake后的opencv库已经分别进行了64位的debug和release的VC12库编译。<br>
直接下载进行VS配置即可。VS版本为VS2012或VS2013 <br>

具体配置如下：<br>
下载该文件<br>
将文件放置自己喜欢的位置。<br>
先进行系统变量的修改。<br>
反击我的电脑->属性->高级系统设置->环境变量<br>
在下面系统变量中找到PATH<br>
然后在里面添加 *\vc12\bin 比如<br>
D:\opencv3\opencv3.0+contrib+vc12+debug64+release64\x64\vc12\bin <br>
弄完环境变量，要记得重启电脑<br>
<br>
接下来打开VS<br>
创建一个win32控制台的工程<br>
调出工程属性管理器<br>
选择debug|x64或release|x64<br>
反击该文件下的Microsoft.Cpp.Win64.user<br>
选择通用属性->VC++目录，在包含目录里添加3个目录：<br>

            E:\opencv2.4.13\opencv\build\include\opencv2<br>

            E:\opencv2.4.13\opencv\build\include\opencv<br>

            E:\opencv2.4.13\opencv\build\include<br>
同样在VC++目录下，在库目录里添加1个目录：E:\opencv2.4.13\opencv\build\x86\vc12\lib<br>
选择通用属性->链接器->输入->附加依赖项，在附加依赖项里添加库文件<br>
opencv_aruco320.lib
opencv_bgsegm320.lib
opencv_bioinspired320.lib
opencv_calib3d320.lib
opencv_ccalib320.lib
opencv_core320.lib
opencv_datasets320.lib
opencv_dnn320.lib
opencv_dpm320.lib
opencv_face320.lib
opencv_features2d320.lib
opencv_flann320.lib
opencv_fuzzy320.lib
opencv_highgui320.lib
opencv_imgcodecs320.lib
opencv_imgproc320.lib
opencv_line_descriptor320.lib
opencv_ml320.lib
opencv_objdetect320.lib
opencv_optflow320.lib
opencv_phase_unwrapping320.lib
opencv_photo320.lib
opencv_plot320.lib
opencv_reg320.lib
opencv_rgbd320.lib
opencv_saliency320.lib
opencv_shape320.lib
opencv_stereo320.lib
opencv_stitching320.lib
opencv_structured_light320.lib
opencv_superres320.lib
opencv_surface_matching320.lib
opencv_text320.lib
opencv_tracking320.lib
opencv_video320.lib
opencv_videoio320.lib
opencv_videostab320.lib
opencv_xfeatures2d320.lib
opencv_ximgproc320.lib
opencv_xobjdetect320.lib
opencv_xphoto320.lib
以上为release64的lib文件，debug均在文件最后添加d<br>
致此就已经配置完成了。<br>

