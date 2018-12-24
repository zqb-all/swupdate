======================
Suricatta 守护进程模式
======================

介绍
------------

和mongoose一样，Suricatta也是SWUpdate的守护模式，
因此得名Suricatta (engl. meerkat)，因为它属于mongoose科。

Suricatta定期轮询远程服务器以获取更新、下载和安装它们。
然后，它重新引导系统，并根据当前存储在引导装载程序环境中的，可持
久化的，在reboot之后仍存在的更新状态变量，向服务器报告更新状态。
可以使用一些U-Boot脚本逻辑或U-Boot的 ``bootcount`` 特性来更改
这个更新状态变量，例如，将其设置为，在启动新刷写的根文件系统失败，
并需要执行回退的时候进行设置以表明更新失败。

在可支持的服务器方面，Suricatta被设计成可扩展的，
如 `支持不同的服务器`_ 部分所述。目前对 `hawkbit`_ 服务器的支持是
通过 `hawkBit Direct Device Integration API`_ 实现的。

.. _hawkBit Direct Device Integration API:  http://sp.apps.bosch-iot-cloud.com/documentation/developerguide/apispecifications/directdeviceintegrationapi.html
.. _hawkBit:  https://projects.eclipse.org/projects/iot.hawkbit


运行suricatta
-----------------

在配置和编译了启用suricatta的SWUpdate之后，

.. code::

  ./swupdate --help

列出使用hawkBit作为服务器时提供给suricatta的强制参数和可选参数。
作为一个例子，

.. code:: bash

    ./swupdate -l 5 -u '-t default -u http://10.0.0.2:8080 -i 25'

使用日志级别 ``TRACE`` 运行SWUpdate的suricatta守护模式，
在suricatta守护进程模式下使用日志级别的“TRACE”运行SWUpdate，
runs SWUpdate in suricatta daemon mode with log-level ``TRACE`` ,
在 ``http://10.0.0.2:8080`` 使用租户 ``default`` 和设备ID ``25`` 轮询hawkBit实例，

注意，安装了更新之后，在启动时，suricatta试图在进入等待进一步更新的主循环之前，
将更新状态报告给其上游服务器，例如hawkBit。
如果初始报告失败，例如，由于未配置的网络或当前不可用的hawkBit服务器，
SWUpdate可能会退出，并带有相应的错误代码。
例如，这种行为允许顺序地尝试几个上游服务器。
如果需要让suricatta一直重试，直到更新状态报告给其上游服务器为止，
而不考虑错误条件，那么必须在外部实现退出时重新启动SWUpdate。

执行更新之后，监听进度接口的代理可能在接收到 ``DONE`` 后执行更新后操作，
例如重新启动。
此外，还可以执行配置文件中指定的或由 ``-p`` 命令行选项给出的更新后命令。

请注意，至少要将SWUpdate的重启作为更新后操作执行，因为只有这样
suricatta才会尝试将更新状态报告给其上游服务器。
否则，上游服务器发布的更新操作将被跳过，并带有一条根据消息，
直到重新启动SWUpdate，以避免安装相同的更新。


支持不同的服务器
----------------------------

通过实现 ``include/channel`` 和 ``include/suricatta/server.h`` 中
描述的 "interface"，可以实现对hawkBit以外的服务器的支持。
前者抽象到服务器的特定连接，例如hawkBit中基于http的连接，
而后者实现轮询和安装更新的逻辑。
可以查看 ``corelib/channel_curl.c``/``include/channel_curl.h`` 和
``suricatta/server_hawkbit.{c,h}`` ,这是一个针对hawkBit的示例实现。


``include/channel.h`` 描述了通道必须实现的功能。

.. code:: c

    typedef struct channel channel_t;
    struct channel {
        ...
    };

    channel_t *channel_new(void);

它设置并返回一个 ``channel_t`` 结构体，该结构体具有指针，
指向通过通道打开、关闭、获取和发送数据的函数。

``include/suricatta/server.h`` 描述服务器必须实现的功能:

.. code:: c

    server_op_res_t server_has_pending_action(int *action_id);
    server_op_res_t server_install_update(void);
    server_op_res_t server_send_target_data(void);
    unsigned int server_get_polling_interval(void);
    server_op_res_t server_start(const char *cfgfname, int argc, char *argv[]);
    server_op_res_t server_stop(void);
    server_op_res_t server_ipc(int fd);

类型 ``server_op_res_t`` 定义在 ``include/suricatta/suricatta.h``.
它表示服务器实现的有效函数返回码。

除了实现特定的通道和服务器， ``suricatta/Config.in`` 文件中必须包含
一个新选项，以便在SWUpdate的配置中可以选择新的实现。
在最简单的情况下，在 ``menu "Server"`` 部分中为hawkBit添加如下选项就足够了。

.. code:: bash

    config SURICATTA_HAWKBIT
        bool "hawkBit support"
        depends on HAVE_LIBCURL
        depends on HAVE_JSON_C
        select JSON
        select CURL
        help
          Support for hawkBit server.
          https://projects.eclipse.org/projects/iot.hawkbit

将新的服务器实现包含到配置中之后，编辑 ``suricatta/Makefile`` 来
指定实现与SWUpdate二进制文件的链接，例如，对于hawkBit示例实现，
以下几行添加了 ``server_hawkbit.o`` 。
如果在配置SWUpdate时选择了 ``SURICATTA_HAWKBIT`` ，则将结果包含
到SWUpdate二进制文件中。

.. code:: bash

    ifneq ($(CONFIG_SURICATTA_HAWKBIT),)
    lib-$(CONFIG_SURICATTA) += server_hawkbit.o
    endif


支持通用HTTP服务器
---------------------------------------

这是一个非常简单的后端，如果更新可用，它将使用标准HTTP响应代码发出信号。
有一些实现此接口的闭源后端，但是由于该接口非常简单，
所以这种服务器类型也适合实现自己的后端服务器。

该API由一个带有查询参数的GET组成，用于通知服务器已安装的版本。
查询字符串的格式为:

::

        http(s)://<base URL>?param1=val1&param2=value2...

作为参数示例，设备可以发送序列号、MAC地址和软件运行版本。
后端需负责对此进行解释——SWUpdate只是从配置文件的 "identity" 部分获取它们，并对URL进行编码。

服务器用以下返回码作出应答:

+-----------+-------------+------------------------------------------------------------+
| HTTP 码   | 文本        | 描述                                                       |
+===========+=============+============================================================+
|    302    | Found       | 在位置标头的URL中有一个新软件可用                          |
+-----------+-------------+------------------------------------------------------------+
|    400    | Bad Request | 一些查询参数丢失或格式错误                                 |
+-----------+-------------+------------------------------------------------------------+
|    403    | Forbidden   | 客户端证书无效                                             |
+-----------+-------------+------------------------------------------------------------+
|    404    | Not found   | 此设备没有更新可用                                         |
+-----------+-------------+------------------------------------------------------------+
|    503    | Unavailable | 更新是可用的，但服务器现在不能处理另一个更新进程。         |
+-----------+-------------+------------------------------------------------------------+

服务器的应答可包含以下头部:

+---------------+--------+------------------------------------------------------------+
| 头部名字      | 代码   | 描述                                                       |
+===============+========+============================================================+
| Retry-after   |   503  | 包含一个数字，该数字告诉设备，在下一次请求更新之前，       |
|               |        | 需要等待多长时间(秒)                                       |
+---------------+--------+------------------------------------------------------------+
| Content-MD5   |   302  | 包含更新文件的校验和，该校验和在位置标头的url下可用        |
+---------------+--------+------------------------------------------------------------+
| Location      |   302  | 可以下载更新文件的URL                                      |
+---------------+--------+------------------------------------------------------------+

设备可以向服务器发送日志数据。任何信息都是在HTTP PUT请求中传输的，消息体中的数据是纯字符串。
内容类型标题需要设置为 text/plain

日志记录的URL可以在配置文件中设置为单独的URL，也可以通过--logurl命令行参数:

设备以CSV格式(逗号分隔的值)发送数据。格式是:

::

        value1,value2,...

可以在配置文件中指定格式。每个 *event* 都可以设置 *format* ，支持的事件有:

+---------------+------------------------------------------------------------+
| Event         | 描述                                                       |
+===============+============================================================+
| check         | dummy。它可以在每次轮询服务器时发送一个事件。              |
+---------------+------------------------------------------------------------+
| started       | 找到一个新软件，SWUpdate开始安装它                         |
+---------------+------------------------------------------------------------+
| success       | 新软件安装成功                                             |
+---------------+------------------------------------------------------------+
| fail          | 新软件安装失败                                             |
+---------------+------------------------------------------------------------+

`general server` 在配置文件中有自己的部分。如下例:

::

        gservice =
        {
	        url 		= ....;
	        logurl		= ;
	        logevent : (
		        {event = "check"; format="#2,date,fw,hw,sp"},
		        {event = "started"; format="#12,date,fw,hw,sp"},
		        {event = "success"; format="#13,date,fw,hw,sp"},
		        {event = "fail"; format="#14,date,fw,hw,sp"}
	        );
        }


`date` 是一个特殊的字段，它被解释为RFC 2822格式的localtime。
在配置文件的 `identify` 部分中查找逗号分隔的每个字段，如果找到匹配项，就进行替换。
如果没有匹配，字段将按原样发送。例如，如果identify部分具有以下值:

::

        identify : (
        	{ name = "sp"; value = "333"; },
        	{ name = "hw"; value = "ipse"; },
        	{ name = "fw"; value = "1.0"; }
        );

配合上述事件设置，"success" 的格式化文本将会是:

::

        Formatted log: #13,Mon, 17 Sep 2018 10:55:18 CEST,1.0,ipse,333
