# read me if you want to config it 
opencv3.0+contrib+vc12+debug64+release64
opencv3.0加上contrib额外库进行cmake后的opencv库已经分别进行了64位的debug和release的VC12库编译。<br>
直接下载进行VS配置即可。VS版本为VS2012或VS2013 <br>

The specific configuration is as follows: <br>
Download the file <br>
Place the files to where you like. <br>
First of all, we should change the system variables. <br>
Counterattack my computer -> attribute -> Advanced System Settings -> environment variables<br>
find the PATH from System variables<br>
then add the path *\vc12\bin for example:<br>
D:\opencv3\opencv3.0+contrib+vc12+debug64+release64\x64\vc12\bin <br>
after it, resetting your computer<br>
<br>
open your VS software<br>
build a project of WIN32 console<br>
choose the property manager<br>
click debug|x64 or release|x64<br>
Counterattack the Microsoft.Cpp.Win64.user<br>
General property ->VC++ directory，add three path to include directory, for example<br>

            D:\opencv3\opencv3.0+contrib+vc12+debug64+release64\build\include\opencv2<br>

            D:\opencv3\opencv3.0+contrib+vc12+debug64+release64\build\include\opencv<br>

            D:\opencv3\opencv3.0+contrib+vc12+debug64+release64\build\include<br>
add path to library directory：D:\opencv3\opencv3.0+contrib+vc12+debug64+release64\build\x64\vc12\lib<br>
General properties - > linker - > Input - > additional dependencies，add the .lib files<br>
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
Above are the files of releasex64，the debug files should add "d" to the tail<br>
Now the configuration is now complete! <br>

