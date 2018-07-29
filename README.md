# tcp_server

一.  编译及运行流程
进入tcp_server文件夹
1. make
2. ./tcp_server or ./tcp_server port. port 是自定义端口，一般要大于1024

二. tcp session 设计说明
   1. tcp session在文件session.h个session.cpp中(类名为: Session).
   2. 客户端连接到来时，首先检测是否有可用的session，如果没有则创建一个新的session。
   3. session处理完毕后，回收session。避免重复申请内存和创建线程
