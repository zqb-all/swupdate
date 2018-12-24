===================================
SWUpdate:用于外部程序的API
===================================

概览
========

SWUpdate包含一个集成的web服务器，可实现远程更新。
但是，在更新过程中涉及的协议是特定于项目的，并且有很大的不同。
有些项目可以决定使用FTP从外部服务器加载映像，或者可以使用专有协议。
集成的web服务器使用这个接口。

SWUpdate有一个简单的接口，可以让外部程序与安装程序通信。
客户端可以启动升级并将镜像流式传递到安装程序，然后查询状态和最终结果。
API目前非常简单，但是如果将来出现新的用例，它可以很容易地进行扩展。

API描述
===============

通信通过UDS (Unix域套接字)运行。套接字在启动时由SWUpdate按照配置创建，
默认是/tmp/sockinstctrl。但是，这个套接字不应该直接使用，而应该由下述客户端库使用。

交换的包在network_ipc.h中描述

::

	typedef struct {
		int magic;
		int type;
		msgdata data;
	} ipc_message;


字段的含义:

- magic : 魔术数字作为包的简单证明
- type : REQ_INSTALL, ACK, NACK, GET_STATUS, POST_UPDATE之一
- msgdata : 客户端用来发送镜像或SWUpdate用来报告通知和状态的缓冲区。

客户端发送一个REQ_INSTALL包并等待应答。
如果更新已经在进行中，SWUpdate将返回ACK或NACK。

在ACK之后，客户端将整个图像作为流发送。SWUpdate希望ACK之后的所有字节
都是要安装的镜像的一部分。SWUpdate从CPIO标头识别图像的大小。
任何错误都将使得SWUpdate退出更新状态，并且在接收到新的REQ_INSTALL之前，
将忽略其他数据包。

.. image:: images/API.png

客户端库
==============

库简化了IPC的使用，提供了异步启动更新的方法。

这个库由一个函数和几个回调组成。

::

        int swupdate_async_start(writedata wr_func, getstatus status_func,
                terminated end_func)
        typedef int (*writedata)(char **buf, int *size);
        typedef int (*getstatus)(ipc_message *msg);
        typedef int (*terminated)(RECOVERY_STATUS status);

swupdate_async_start创建一个新线程，并启动与SWUpdate的通信，触发新的更新。
wr_func会被调用，以获取要安装的镜像。回调函数负责提供缓冲区和数据块的大小。

流下载完毕后，可调用getstatus回调，以检查升级进行的状况。
如果只需要结果，则可以省略。

当SWUpdate完成升级后，将调用terminated回调。

关于使用这个库的示例，在examples/client目录中。
