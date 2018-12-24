在运行更新时获取信息
=====================================

通常需要告知操作员运行更新的状态，而不仅仅是返回更新是否成功。
例如，如果目标具有显示或远程接口，则可以转发当前更新的目标及百分比，
以便估计还有多少更新需要完成。

SWUpdate为此提供了一个接口(“progress API”)。
外部进程可以在SWUpdate中注册自己，
当更新中的某些内容发生更改时，它将接收通知。
这与IPC API不同，因为后者主要用于传输SWU镜像，
只有轮询接口才能知道更新是否仍在运行。


API 描述
---------------

外部进程将自己注册到SWUpdate，并向SWUpdate默认配置的域套接字
"/tmp/swupdateprog" 发出connect()请求。
当没有要发送的信息时，SWUpdate只是将新连接插入要通知的进程列表中。
SWUpdate在更新过程中发生任何更改后，将使用以下数据
(请参见include/progress_ipc.h)发回一帧数据:

::

        struct progress_msg {
        	unsigned int	magic;		/* Magic Number */
        	unsigned int	status;		/* Update Status (Running, Failure) */
        	unsigned int	dwl_percent;	/* % downloaded data */
        	unsigned int	nsteps;		/* No. total of steps */
        	unsigned int	cur_step;	/* Current step index */
        	unsigned int	cur_percent;	/* % in current step */
        	char		cur_image[256];	/* Name of image to be installed */
        	char		hnd_name[64];	/* Name of running handler */
        	sourcetype	source;		/* Interface that triggered the update */
        	unsigned int 	infolen;    	/* Len of data valid in info */
        	char		info[2048];   	/* additional information about install */
        };

单个字段的含义如下:

        - *magic* 尚未使用，它可以被添加为帧的简单验证。
        - *status* 是swupdate_status.h中的值之一(START, RUN, SUCCESS, FAILURE, DOWNLOAD, DONE)。
        - *dwl_percent* 是status = Download 时下载数据的百分比。
        - *nsteps* 是要运行的安装程序(处理程序)的总数。
        - *cur_step* 是正在运行的处理程序的索引。cur_step在1..nsteps范围内
        - *cur_percent* 是在当前处理程序中完成的工作的百分比。这在通过慢速接口，如低速flash,
          进行更新时非常有用。信号是镜像已经复制到目标的百分比。
        - *cur_image* 是当前正在安装的镜像在sw-description中的名称。
        - *hnd_name* 报告正在运行的处理程序的名称。
        - *source* 是触发更新的接口。
        - *infolen* 后续info字段的数据长度。
        - *info* 关于安装的附加信息。

进度客户端的一个例子是 ``tools/progress`` 。
在控制台打印状态，并驱动 "psplash" 在屏幕上绘制进度条。
