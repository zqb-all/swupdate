=============================================
处理程序
=============================================

概览
--------

很难预见所有可能的安装情况。
SWUpdate不尝试找出并支持所有用例，
而是让开发人员能自由添加自己的安装程序(即新的 **处理程序** )，
它必须负责安装某种类型的镜像。
镜像被标记为一种已定义的类型，将使用特定的处理程序安装。

解析器在 '镜像类型' 和 '处理程序' 之间建立连接。
它维护一个表用于执行安装，其中包含要安装的镜像列表和处理程序。
每个镜像可以有不同的安装程序。

已提供的处理程序
-----------------

主线代码中有用于最常见情况的处理程序。它们包括:
	- 裸数据模式的flash设备(NOR 和 NAND)
	- UBI卷
	- 裸设备，例如一个SD卡分区
	- 启动引导程序 (U-Boot, GRUB, EFI Boot Guard) 的环境变量
	- Lua脚本

例如，如果将图像标记为要更新到UBI卷，解析器必须在维护的表中设置 "ubi"
为需要的处理程序，并填充此处理程序所需的其他字段：卷名、大小等等。

创建自己的处理程序
---------------------

SWUpdate可以使用新的处理程序进行扩展。
用户需要向核心注册自己的处理程序，并且必须提供SWUpdate在需要使用新处理程序
安装镜像时使用的回调。

回调的原型是:

::

	int my_handler(struct img_type *img,
		void __attribute__ ((__unused__)) *data)


最重要的参数是指向struct img_type的指针。
它描述单个镜像并通知处理程序镜像必须安装在何处。
输入流的文件描述符会设置为安装镜像的开始，这也是结构的一部分。

结构 *img_type* 包含指向要安装的镜像的第一个字节的流的文件描述符。
处理程序必须读取整个镜像，当它返回时，
SWUpdate可以继续处理流中的下一个镜像。

SWUpdate提供了一个通用函数来从流中提取数据并复制到其他地方:

::

        int copyfile(int fdin, int fdout, int nbytes, unsigned long *offs,
                int skip_file, int compressed, uint32_t *checksum, unsigned char *hash);

fdin是输入流，即来自回调的img->fdin。对于签名镜像，只需将 *hash* 传递
给copyfile()来执行检查，就像 *checksum* 参数一样。如果校验和或散列不匹配，
copyfile()将返回一个错误。处理程序不需要为它们费心。

处理程序如何管理复制的数据，是特定于处理程序本身的。
请参阅提供的处理程序代码以更好地进行理解。

处理程序的开发人员使用以下调用方式注册自己的处理程序：

::

	__attribute__((constructor))
	void my_handler_init(void)
	{
		register_handler("mytype", my_handler, my_mask, data);
	}

SWUpdate使用gcc构造函数，并且在初始化SWUpdate时注册所有提供的处理程序。

register_handler的语法如下:

::

	register_handler(my_image_type, my_handler, my_mask, data);

其中:

- my_image_type : 标识自己的新镜像类型的字符串。
- my_handler :指向要注册的安装程序的指针。
- my_mask : ``HANDLER_MASK`` 枚举值，指定 my_handler 可处理什么输入类型。
- data : 一个可选的指向自定义结构的指针。SWUpdate会将其保存在处理程序
  的列表中，并在执行时传递给处理程序。

UBI卷处理程序
-----------------------

UBI卷的处理程序被认为可以在不改变存储布局的情况下更新UBI卷。
卷必须提前设置:处理程序本身不创建卷。它在所有MTD中搜索一个卷
(如果它们没有被列入黑名单:请参阅 UBIBLACKLIST)，以找到要安装映像的卷。
因此，卷在系统内必须是惟一的。
不支持名称相同的两个卷，这会导致不可预知的结果。
SWUpdate将安装镜像到名称匹配的第一个卷，这可能不是期望的行为。

更新卷时，可以保证擦除计数器在更新后不会丢失。
更新的方式与来自mtd-utils的 "ubiupdatevol" 相同。
事实上上，SWUpdate重用了来自mtd-utils (libubi)的同一个库。

SWUpdate通常创建动态卷。如果需要静态卷，请将处理程序的数据字段
设置为 "static"。

如果存储为空，则需要设置布局并创建卷。这可以通过预安装脚本轻松完成。
使用meta-SWUpdate进行构建时，原始的mtd-utils是可用的，
并且可以由Lua脚本调用。

Lua Handlers
------------

In addition to the handlers written in C, it is possible to extend
SWUpdate with handlers written in Lua that get loaded at SWUpdate
startup. The Lua handler source code file may either be embedded
into the SWUpdate binary via the ``CONFIG_EMBEDDED_LUA_HANDLER``
config option or has to be installed on the target system in Lua's
search path as ``swupdate_handlers.lua`` so that it can be loaded
by the embedded Lua interpreter at run-time.

In analogy to C handlers, the prototype for a Lua handler is

::

        function lua_handler(image)
            ...
        end

where ``image`` is a Lua table (with attributes according to
:ref:`sw-description's attribute reference <sw-description-attribute-reference>`)
that describes a single artifact to be processed by the handler. 

Note that dashes in the attributes' names are replaced with
underscores for the Lua domain to make them idiomatic, e.g.,
``installed-directly`` becomes ``installed_directly`` in the
Lua domain.

To register a Lua handler, the ``swupdate`` module provides the
``swupdate.register_handler()`` method that takes the handler's
name, the Lua handler function to be registered under that name,
and, optionally, the types of artifacts for which the handler may
be called. If the latter is not given, the Lua handler is registered
for all types of artifacts. The following call registers the
above function ``lua_handler`` as *my_handler* which may be
called for images:

::

        swupdate.register_handler("my_handler", lua_handler, swupdate.HANDLER_MASK.IMAGE_HANDLER)


A Lua handler may call C handlers ("chaining") via the
``swupdate.call_handler()`` method. The callable and registered
C handlers are available (as keys) in the table
``swupdate.handler``. The following Lua code is an example of
a simple handler chain-calling the ``rawfile`` C handler:

::

        function lua_handler(image)
            if not swupdate.handler["rawfile"] then
                swupdate.error("rawfile handler not available")
                return 1
            end
            image.path = "/tmp/destination.path"
            local err, msg = swupdate.call_handler("rawfile", image)
            if err ~= 0 then
                swupdate.error(string.format("Error chaining handlers: %s", msg))
                return 1
            end
            return 0
        end

Note that when chaining handlers and calling a C handler for
a different type of artifact than the Lua handler is registered
for, the ``image`` table's values must satisfy the called
C handler's expectations: Consider the above Lua handler being
registered for "images" (``swupdate.HANDLER_MASK.IMAGE_HANDLER``)
via the ``swupdate.register_handler()`` call shown above. As per the 
:ref:`sw-description's attribute reference <sw-description-attribute-reference>`,
the "images" artifact type doesn't have the ``path`` attribute
but the "file" artifact type does. So, for calling the ``rawfile``
handler, ``image.path`` has to be set prior to chain-calling the
``rawfile`` handler, as done in the example above. Usually, however,
no such adaptation is necessary if the Lua handler is registered for
handling the type of artifact that ``image`` represents.

In addition to calling C handlers, the ``image`` table passed as
parameter to a Lua handler has a ``image:copy2file()`` method that
implements the common use case of writing the input stream's data
to a file, which is passed as this method's argument. On success,
``image:copy2file()`` returns ``0`` or ``-1`` plus an error
message on failure. The following Lua code is an example of
a simple handler calling ``image:copy2file()``:

::

        function lua_handler(image)
            local err, msg = image:copy2file("/tmp/destination.path")
            if err ~= 0 then
                swupdate.error(string.format("Error calling copy2file: %s", msg))
                return 1
            end
            return 0
        end

Beyond using ``image:copy2file()`` or chain-calling C handlers,
the ``image`` table passed as parameter to a Lua handler has
a ``image:read(<callback()>)`` method that reads from the input
stream and calls the Lua callback function ``<callback()>`` for
every chunk read, passing this chunk as parameter. On success,
``0`` is returned by ``image:read()``. On error, ``-1`` plus an
error message is returned. The following Lua code is an example
of a simple handler printing the artifact's content:

::

        function lua_handler(image)
            err, msg = image:read(function(data) print(data) end)
            if err ~= 0 then
                swupdate.error(string.format("Error reading image: %s", msg))
                return 1
            end
            return 0
        end

Using the ``image:read()`` method, an artifact's contents may be
(post-)processed in and leveraging the power of Lua without relying
on preexisting C handlers for the purpose intended.


Just as C handlers, a Lua handler must consume the artifact 
described in its ``image`` parameter so that SWUpdate can 
continue with the next artifact in the stream after the Lua handler
returns. Chaining handlers, calling ``image:copy2file()``, or using 
``image:read()`` satisfies this requirement.


Note that although the dynamic nature of Lua handlers would
technically allow to embed them into a to be processed ``.swu``
image, this is not implemented as it carries some security
implications since the behavior of SWUpdate is changed
dynamically.

Remote handler
--------------

Remote handlers are thought for binding legacy installers
without having the necessity to rewrite them in Lua. The remote
handler forward the image to be installed to another process,
waiting for an acknowledge to be sure that the image is installed
correctly.
The remote handler makes use of the zeromq library - this is
to simplify the IPC with Unix Domain Socket. The remote handler
is quite general, describing in sw-description with the
"data" attribute how to communicate with the external process.
The remote handler always acts as client, and try a connect()
using the socket identified by the "data" attribute. For example,
a possible setup using a remote handler could be:

::

        images: (
                {
                    filename = "myimage"";
                    type = "remote";
                    data = "test_remote";
                 }
        )


The connection is instantiated using the socket "/tmp/test_remote". If
connect() fails, the remote handler signals that the update is not successful.
Each Zeromq Message from SWUpdate is a multi-part message split into two frames:

        - first frame contains a string with a command.
        - second frame contains data and can be of 0 bytes.

There are currently just two possible commands: INIT and DATA. After
a successful connect, SWUpdate sends the initialization string in the
format:


::
        
        INIT:<size of image to be installed>

The external installer is informed about the size of the image to be
installed, and it can assign resources if it needs. It will answer
with the string *ACK* or *NACK*. The first NACK received by SWUpdate
will interrupt the update. After sending the INIT command, the remote
handler will send a sequence of *DATA* commands, where the second
frame in message will contain chunks of the image to be installed.
It is duty of the external process to take care of the amount of
data transferred and to release resources when the last chunk
is received. For each DATA message, the external process answers with a
*ACK* or *NACK* message.

SWU forwarder
---------------

The SWU forwarder handler can be used to update other systems where SWUpdate
is running. It can be used in case of master / slaves systems, where the master
is connected to the network and the "slaves" are hidden to the external world.
The master is then the only interface to the world. A general SWU can contain
embedded SWU images as single artifacts, and the SWU handler will forward it
to the devices listed in the description of the artifact.
The handler can have a single "url" properties entry with an array of urls. Each url
is the address of a secondary board where SWUpdate is running with webserver activated.
The SWU handler expects to talk with SWUpdate's embedded webserver. This helps
to update systems where an old version of SWUpdate is running, because the
embedded webserver is a common feature present in all versions.
The handler will send the embedded SWU to all URLs at the same time, and setting
``installed-directly`` is supported by this handler.

.. image:: images/SWUGateway.png

The following example shows how to set a SWU as artifact and enables
the SWU forwarder:


::

	images: (
		{
			filename = "image.swu";
			type = "swuforward";

			properties: {
				url = ["http://192.168.178.41:8080", "http://192.168.178.42:8080"];
			};
		});

ucfw handler
------------

This handler allows to update the firmware on a microcontroller connected to
the main controller via UART.
Parameters for setup are passed via sw-description file.  Its behavior can be
extended to be more general.
The protocol is ASCII based. There is a sequence to be done to put the microcontroller
in programming mode, after that the handler sends the data and waits for an ACK from the
microcontroller.

The programming of the firmware shall be:

1. Enter firmware update mode (bootloader)

        1. Set "reset line" to logical "low"
	2. Set "update line" to logical "low"
	3. Set "reset line" to logical "high"

2. Send programming message

::

        $PROG;<<CS>><CR><LF>

to the microcontroller.  (microcontroller will remain in programming state)

3. microcontroller confirms with

::

        $READY;<<CS>><CR><LF>

        4. Data transmissions package based from mainboard to microcontroller
package definition:

        - within a package the records are sent one after another without the end of line marker <CR><LF>
        - the package is completed with <CR><LF>

5. The microcontroller requests the next package with $READY;<<CS>><CR><LF>

6. Repeat step 4 and 5 until the complete firmware is transmitted.

7. The keypad confirms the firmware completion with $COMPLETED;<<CS>><CR><LF>

8. Leave firmware update mode
        1. Set "Update line" to logical "high"
        2. Perform a reset over the "reset line"

<<CS>> : checksum. The checksum is calculated as the two's complement of
the modulo-256 sum over all bytes of the message
string except for the start marker "$".
The handler expects to get in the properties the setup for the reset
and prog gpios. They should be in this format:

::

        properties = {
	        reset = "<gpiodevice>:<gpionumber>:<activelow>";
                prog = "<gpiodevice>:<gpionumber>:<activelow>";
        }

Example:

::

        properties = {
                reset =  "/dev/gpiochip0:38:false";
                prog =  "/dev/gpiochip0:39:false";
        }

