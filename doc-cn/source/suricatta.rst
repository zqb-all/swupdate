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
如 `支持不同服务器`_ 部分所述。目前对 `hawkbit`_ 服务器的支持是
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


Supporting different Servers
----------------------------

Support for servers other than hawkBit can be realized by implementing
the "interfaces" described in ``include/channel.h`` and
``include/suricatta/server.h``. The former abstracts a particular
connection to the server, e.g., HTTP-based in case of hawkBit, while
the latter implements the logics to poll and install updates.
See ``corelib/channel_curl.c``/``include/channel_curl.h`` and
``suricatta/server_hawkbit.{c,h}`` for an example implementation
targeted towards hawkBit.

``include/channel.h`` describes the functionality a channel
has to implement:

.. code:: c

    typedef struct channel channel_t;
    struct channel {
        ...
    };

    channel_t *channel_new(void);

which sets up and returns a ``channel_t`` struct with pointers to
functions for opening, closing, fetching, and sending data over
the channel.

``include/suricatta/server.h`` describes the functionality a server has
to implement:

.. code:: c

    server_op_res_t server_has_pending_action(int *action_id);
    server_op_res_t server_install_update(void);
    server_op_res_t server_send_target_data(void);
    unsigned int server_get_polling_interval(void);
    server_op_res_t server_start(const char *cfgfname, int argc, char *argv[]);
    server_op_res_t server_stop(void);
    server_op_res_t server_ipc(int fd);

The type ``server_op_res_t`` is defined in ``include/suricatta/suricatta.h``.
It represents the valid function return codes for a server's implementation.

In addition to implementing the particular channel and server, the
``suricatta/Config.in`` file has to be adapted to include a new option
so that the new implementation becomes selectable in SWUpdate's
configuration. In the simplest case, adding an option like the following
one for hawkBit into the ``menu "Server"`` section is sufficient.

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

Having included the new server implementation into the configuration,
edit ``suricatta/Makefile`` to specify the implementation's linkage into
the SWUpdate binary, e.g., for the hawkBit example implementation, the
following lines add ``server_hawkbit.o`` to the resulting SWUpdate binary
if ``SURICATTA_HAWKBIT`` was selected while configuring SWUpdate.

.. code:: bash

    ifneq ($(CONFIG_SURICATTA_HAWKBIT),)
    lib-$(CONFIG_SURICATTA) += server_hawkbit.o
    endif


Support for general purpose HTTP server
---------------------------------------

This is a very simple backend that uses standard HTTP response codes to signal if
an update is available. There are closed source backends implementing this interface,
but because the interface is very simple interface, this server type is also suitable
for implementing an own backend server.

The API consists of a GET with Query parameters to inform the server about the installed version.
The query string has the format:

::

        http(s)://<base URL>?param1=val1&param2=value2...

As examples for parameters, the device can send its serial number, MAC address and the running version of the software.
It is duty of the backend to interprete this - SWUpdate just takes them from the "identity" section of
the configuration file and encodes the URL.

The server answers with the following return codes:

+-----------+-------------+------------------------------------------------------------+
| HTTP Code | Text        | Description                                                |
+===========+=============+============================================================+
|    302    | Found       | A new software is available at URL in the Location header  |
+-----------+-------------+------------------------------------------------------------+
|    400    | Bad Request | Some query parameters are missing or in wrong format       |
+-----------+-------------+------------------------------------------------------------+
|    403    | Forbidden   | Client certificate not valid                               |
+-----------+-------------+------------------------------------------------------------+
|    404    | Not found   | No update is available for this device                     |
+-----------+-------------+------------------------------------------------------------+
|    503    | Unavailable | An update is available but server can't handle another     |
|           |             | update process now.                                        |
+-----------+-------------+------------------------------------------------------------+

Server's answer can contain the following headers:

+---------------+--------+------------------------------------------------------------+
| Header's name | Codes  | Description                                                |
+===============+========+============================================================+
| Retry-after   |   503  | Contains a number which tells the device how long to wait  |
|               |        | until ask the next time for updates. (Seconds)             |
+---------------+--------+------------------------------------------------------------+
| Content-MD5   |   302  | Contains the checksum of the update file which is available|
|               |        | under the url of location header                           |
+---------------+--------+------------------------------------------------------------+
| Location      |   302  | URL where the update file can be downloaded.               |
+---------------+--------+------------------------------------------------------------+

The device can send logging data to the server. Any information is transmitted in a HTTP
PUT request with the data as plain string in the message body. The Content-Type Header
need to be set to text/plain.

The URL for the logging can be set as separate URL in the configuration file or via
--logurl command line parameter:

The device sends data in a CSV format (Comma Separated Values). The format is:

::

        value1,value2,...

The format can be specified in the configuration file. A *format* For each *event* can be set.
The supported events are:

+---------------+------------------------------------------------------------+
| Event         | Description                                                |
+===============+========+===================================================+
| check         | dummy. It could send an event each time the server is      |
|               | polled.                                                    |
+---------------+------------------------------------------------------------+
| started       | A new software is found and SWUpdate starts to install it  |
+---------------+------------------------------------------------------------+
| success       | A new software was successfully installed                  |
+---------------+------------------------------------------------------------+
| fail          | Failure by installing the new software                     |
+---------------+------------------------------------------------------------+

The `general server` has an own section inside the configuration file. As example:

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


`date` is a special field and it is interpreted as localtime in RFC 2822 format. Each
Comma Separated field is looked up inside the `identify` section in the configuration
file, and if a match is found the substitution occurs. In case of no match, the field
is sent as it is. For example, if the identify section has the following values:


::

        identify : (
        	{ name = "sp"; value = "333"; },
        	{ name = "hw"; value = "ipse"; },
        	{ name = "fw"; value = "1.0"; }
        );


with the events set as above, the formatted text in case of "success" will be:

::

        Formatted log: #13,Mon, 17 Sep 2018 10:55:18 CEST,1.0,ipse,333
