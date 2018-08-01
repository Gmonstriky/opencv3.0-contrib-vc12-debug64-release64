# read me if you want to config it 
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
opencv_aruco320.lib<br>
opencv_bgsegm320.lib<br>
opencv_bioinspired320.lib<br>
opencv_calib3d320.lib<br>
opencv_ccalib320.lib<br>
opencv_core320.lib<br>
opencv_datasets320.lib<br>
opencv_dnn320.lib<br>
opencv_dpm320.lib<br>
opencv_face320.lib<br>
opencv_features2d320.lib<br>
opencv_flann320.lib<br>
opencv_fuzzy320.lib<br>
opencv_highgui320.lib<br>
opencv_imgcodecs320.lib<br>
opencv_imgproc320.lib<br>
opencv_line_descriptor320.lib<br>
opencv_ml320.lib<br>
opencv_objdetect320.lib<br>
opencv_optflow320.lib<br>
opencv_phase_unwrapping320.lib<br>
opencv_photo320.lib<br>
opencv_plot320.lib<br>
opencv_reg320.lib<br>
opencv_rgbd320.lib<br>
opencv_saliency320.lib<br>
opencv_shape320.lib<br>
opencv_stereo320.lib<br>
opencv_stitching320.lib<br>
opencv_structured_light320.lib<br>
opencv_superres320.lib<br>
opencv_surface_matching320.lib<br>
opencv_text320.lib<br>
opencv_tracking320.lib<br>
opencv_video320.lib<br>
opencv_videoio320.lib<br>
opencv_videostab320.lib<br>
opencv_xfeatures2d320.lib<br>
opencv_ximgproc320.lib<br>
opencv_xobjdetect320.lib<br>
opencv_xphoto320.lib<br>
以上为release64的lib文件，debug均在文件最后添加d<br>
致此就已经配置完成了。<br>

