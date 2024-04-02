#加一个-g,可执行文件能够进行调试

#配置头文件搜索路径 -I
#include_directories()

#配置库文件搜索路径 -L
#link_directories()
 
#设置可执行文件最终存储的路径  当前工程路径下的bin目录
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)


#设置需要编译的源文件列表   当前路径下
set(SRC_LIST ./muduo_server.cpp)

#把指定路径下的所有源文件名字放入变量名SRC_LIST
#aux_source_directory(. SRC_LIST)

#生成可执行文件server，由SRC_LIST变量所定义的源文件编译而来
add_executable(server ${SRC_LIST})

#server这个目标程序，需要连接muduo_net muduo_base pthread这三个库文件
target_link_libraries(server muduo_net muduo_base pthread)


#之后，使用cmake. 在使用make