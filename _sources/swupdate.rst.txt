=============================================
SWUpdate: 嵌入式系统的软件升级
=============================================

概述
========

本项目被认为有助于从存储媒体或网络更新嵌入式系统。
但是，它应该主要作为一个框架来考虑，在这个框架中可以方便地
向应用程序添加更多的协议或安装程序(在SWUpdate中称为处理程序)。

一个用例是从外部本地媒体(如USB-Pen或sd卡)进行更新。
在这种情况下，更新是在没有操作员干预的情况下完成的:
它被认为是“一键更新”，软件在复位时启动，只需按下一个键
(或者以任何目标可以识别的方式)，自动进行所有检查。
最后，更新过程只向操作员报告状态(成功或失败)。

输出可以使用帧缓冲设备显示在LCD上，
也可以定向到串行通讯端口上(Linux控制台)。

它通常用于单拷贝方案中，在initrd中运行(用Yocto提供的配方生成)。
但是，通过使用软件集合( :ref:`collections` )，可以在双拷贝方案中使用它。

如果启动了远程更新，SWUpdate将启动嵌入式web服务器并等待请求。
操作者必须上传一个合适的映像，然后SWUpdate会进行检查并安装。
所有输出都通过AJAX通知的方式通知操作人员的浏览器。

功能
========

总体概览
----------------

- 安装在嵌入式介质上(eMMC、SD、Raw NAND、NOR、SPI-NOR flash)

- 检查镜像是否可用。镜像以指定的格式(cpio)构建，它必须包含一个描述文件，
  以描述必须更新的软件。

- SWUpdate被认为可以更新设备上的UBI卷(主要用于NAND，
  但不限于NAND)和镜像。传递整个镜像仍然用于对SD卡上
  的分区或MTD分区进行更新。

- 新分区模式。这与UBI容量有关。SWUpdate可以重新创建UBI卷，
  调整它们的大小并复制新软件。一个名为“data”的特殊UBI卷
  在重新分区时，用于保存和恢复数据，以保持好用户数据。

- 使用zlib库支持压缩镜像。支持tarball (tgz文件)。

- 支持带分区的USB-pen或未分区盘(主要用于Windows)。

- 支持更新文件系统中的单个文件。
  必须明确描述该文件所在的文件系统位置。

- 支持图像中单个组件的校验和

- 使用结构化语言来描述镜像。
  这是使用 libconfig_ 库作为缺省解析器完成的，它使用一种类似json的描述。

- 使用自定义的方式来描述镜像。可以使用Lua语言编写自己的解析器。
  examples目录中提供了一个使用Lua中的XML描述的示例。

- 支持设置/删除U-Boot变量

- 支持设置/擦除 `GRUB`_ 环境块变量

- 支持设置/删除 `EFI Boot Guard`_ 变量

- 使用嵌入式web服务器的网络安装程序
  (在Lua许可下的版本中选择了Mongoose服务器)。
  可以使用不同的web服务器。

- 多种获取软件的接口
       - 本地存储: USB, SD, UART,..

- OTA / 远程
       - 集成的网络服务器
       - 从远程服务器拉取(HTTP, HTTPS， ..)
       - 使用后端。SWUpdate是开放的，可以与后端服务器进行通信，
         以推出软件更新。当前版本支持Hawkbit服务器，
         但可以添加其他后端。

- 可以配置为检查软件和硬件之间的兼容性。
  软件映像必须包含条目，声明这个软件可在什么版本硬件上运行。
  如果没有通过兼容性验证，SWUpdate将拒绝安装。

- 支持镜像提取。制造商用一个映像包含用于多个设备的软件。
  这简化了制造商的管理，并降低了单一软件产品的管理成本。
  SWUpdate以流的形式接收软件，不进行临时存储，并只提取需要安装的设备组件。

- 允许自定义处理器，通过自定义协议安装FPGA固件，微控制器固件。

- 使用“make menuconfig”启用/禁用特性。(Kbuild继承自busybox项目)

- 镜像在安装之前经过身份认证和校验

- 掉电安全

.. _libconfig: http://www.hyperrealm.com/libconfig/
.. _GRUB: https://www.gnu.org/software/grub/manual/html_node/Environment-block.html
.. _EFI Boot Guard: https://github.com/siemens/efibootguard

交付单一镜像
---------------------

主要概念是制造商提供单个大图像。
所有单个的镜像都被打包在一起(选择cpio是因为它
的简单性和可流式处理)，同时打包的还有另一个文
件(sw-description)，该文件包含每个独立镜像的元信息。

sw-description的格式是可定制的:可以将SWUpdate配置为
使用其内部解析器(基于libconfig)，或者在调用外部的lua解析器。


.. image:: images/image_format.png

可以使用外部解析器，改变对镜像的接受规则，以扩展支持新的镜像类型，
指明它们需要如何安装。实际上，解析器就是检索必须安装哪些单个的镜像
以及如何安装。

SWUpdate使用“处理程序”来安装单个镜像:
有用于将镜像安装到UBI卷或SD卡、CFI闪存等的处理程序。
如果需要特殊的安装程序，那么也可以很容易地添加自己的处理程序。

例如，我们可以考虑一个带有主处理器和一个或几个微控制器的项目。
为了简单起见，我们假设主处理器使用专用协议通过UARTS与微控制器通信。
微控制器上的软件可以使用专用协议进行更新。

可以扩展swuodate，编写一个处理程序，实现专用协议的一部分
来对微控制器进行升级。解析器必须识别哪个镜像必须用新的处理
程序来安装，随后SWUpdate将在安装过程中调用该处理程序。

流式更新功能
-----------------

SWUpdate被认为能够将接收到的镜像直接流式更新到目标中，
而不需要任何临时副本。实际上，单个安装程序(处理程序)会接收
一个文件描述符作为输入，该文件描述符设置在必须安装的图像的开始处。

该特性可以基于镜像进行设置，这意味着用户可以决定镜像的哪些部分
应该流式处理。如果没有流式处理(请参见installed-direct标志)，
文件将临时提取到环境变量 ``TMPDIR`` 指向的目录中，如果没有
设置 ``TMPDIR`` ，则默认使用 ``/tmp`` 。
当然，使用流式处理，则不可能在安装之前检查整个交付的软件。
临时副本仅在从网络更新时使用。
当映像存储在外部存储上时，不需要该副本。

Images fully streamed
---------------------

在远程更新的情况下，SWUpdate从流中提取相关图像，并将它们复制
到环境变量 ``TMPDIR`` (如果未设置，则复制到 ``/tmp`` )指向的目录中，
然后调用处理程序。这确保只有在所有部件都存在且正确时才会启动更新。
但是，在一些资源较少的系统上，用于复制镜像的RAM空间可能不足，
例如，如果必须更新附加SD卡上的文件系统的话。在这种情况下，如果
图像能由相应的处理程序直接作为流安装，而不需要临时副本的话，
则会很有帮助。并非所有处理程序都支持直接流式更新目标。
零拷贝流是通过在单个镜像像的描述中设置“installed-directly”标志来启用的。

配置和构建
=======================

需求
------------

编译SWUpdate只需要依赖几个库。

- mtd-utils: mtd-utils在内部生成libmtd和libubi。它们通常不导出也不安装，
  但是SWUpdate将链接它们，以便重用相同的功能来升级MTD和UBI卷。
- openssl: web服务器需要。
- Lua: liblua和开发头文件。
- libz和libcrypto总是需要被链接。
- libconfig: 被默认解析器使用。
- libarchive (可选的)用于存档处理程序。
- libjson (可选的)用于JSON解析器和Hawkbit。
- libubootenv (可选的) 如果启用了对U-Boot的支持则需要。
- libebgenv (可选的) 如果启用了对EFI Boot Guard的支持则需要。
- libcurl 用于网络通讯。

新的处理程序可以向需求列表中添加一些其他的库 -
当出现构建错误时，检查是否需要所有的处理程序，然后删除其中不需要的部分。

在Yocto中进行构建
-------------------

提供了一个 meta-swupdate_ 层.它包含了mtd-utils和生成Lua所需的更改。
使用meta-SWUpdate只需一些简单的步骤。

首先，克隆 meta-swupdate.

::

        git clone https://github.com/sbabic/meta-swupdate.git

.. _meta-SWUpdate:  https://github.com/sbabic/meta-swupdate.git

像往常一样向 bblayer.conf 添加 meta-swupdate。
你还需要将 meta-oe 添加到list中。

在meta-swupdate中，有一个配方，用于生成带有swupdate的initrd救援系统。
使用：

::

	MACHINE=<your machine> bitbake swupdate-image

你将在 tmp/deploy/<your machine> 目录中找到生成的结果。
如何安装和启动initrd是跟具体目标强相关的 - 请查阅你的
引导加载程序的文档。

libubootenv呢 ?
------------------------

这是构建SWUpdate时常见的问题。SWUpdate依赖于这个库，
它是从U-Boot源码生成的。这个库允许安全地修改U-Boot环境变量。
如果不使用U-Boot作为引导加载程序，则不需要它。
如果无法SWUpdate正常链接，则你使用的是旧版本的U-Boot
(你至少需要2016.05以上的版本)。如果是这样，你可以为
包u-boot-fw-utils添加自己的配方，以添加这个库的代码。

重要的是，包u-boot-fw-utils是用相同的引导加载程序源码和相同的机器构建的。
事实上，设备可以使用一份直接链接到uboot中的默认环境变量，而不需要保存在
存储器上。SWUpdate应该知道这一点，因为它不能读取这份环境变量:默认的这份
环境变量也必须被链接到SWUpdate中。这是在libubootenv内部完成的。

如果构建的时候选择了不同的机器，SWUpdate将在第一次尝试更改环境变量时
破坏环境变量。实际上，使用了错误的默认环境后，你的板子将不能再次被
引导启动。

配置SWUpdate
--------------------

SWUpdate可以通过“make menuconfig”配置。
使用内部解析器和禁用web服务器可以达到较小的内存占用。
每个选项都有描述其用法的小帮助说明。
在默认配置中，许多选项已经被激活。

要配置选项请执行:

::

	make menuconfig

构建
--------

- 要进行交叉编译，请在运行make之前设置CC和CXX变量。
  也可以使用make menuconfig将交叉编译器前缀设置为选项。
- 生成代码

::

	make

结果时一个二进制文件“swupdate”。第二个构建的二进制文件
是"process"，但这并非严格要求的。这是一个示例，演示如何
构建自己的SWUpdate接口来在HMI上显示进度条或任何你想要的东西。
具体到这个示例，则是简单地在控制台打印更新的当前状态。


在Yocto构建系统中，:

::

        bitbake swupdate

这将进行包的构建

::

        bitbake swupdate-image

这将构建一个救援镜像。
结果是一个可以由引导加载程序直接加载的Ramdisk。
要在双拷贝模式下使用SWUpdate的话，则将包swupdate放到你的rootfs中。
检查你的镜像配方文件，并简单地将其添加到安装包的列表中。

例如，如果我们想将它添加到标准的“core-image-full-cmdline”镜像中，
我们可以添加一个 *recipes-extended/images/core-image-full-cmdline.bbappend*

::

        IMAGE_INSTALL += " \
                                swupdate \
                                swupdate-www \
                         "
swupdate-www是一个带有网站的软件包，你可以用自己的logo、模板
和风格进行定制。

编译一个debian包
-------------------------

SWUpdate被认为是用于嵌入式系统的，在嵌入式发行版中构建
是首要的情况。但是除了最常用的嵌入式构建系统Yocto或
Buildroot之外，在某些情况下还会使用标准的Linux发行版。
不仅如此，发行版包还允许为了测试目的在Linux PC上
运行SWUpdate，而不必与依赖项做斗争。
使用debhelper工具，可以生成debian包。


编译一个debian包的步骤
...................................

::

        ./debian/rules clean
        ./debian/rules build
        fakeroot debian/rules binary

结果是一个存储在父目录中的“deb”包。

对源包签名的替代方法
......................................

你可以使用dpkg-buildpackage:

::

        dpkg-buildpackage -us -uc
        debsign -k <keyId>


运行SWUpdate
================

运行一次swupdate可以期望得到什么
------------------------------------

SWUpdate的运行主要包括以下步骤:

- 检查介质(usb pen)
- 检查镜像文件。扩展名必须是.swu
- 从镜像中提取sw-description并验证它，
  它解析sw-description，在RAM中创建关于必须执行的活动的原始描述。
- 读取cpio归档文件并验证每个文件的校验和，如果归档文件未完全
  通过验证，SWUpdate将停止执行。
- 检查硬件-软件兼容性，如果有的话，从硬件中读取硬件修改，
  并与sw-description中的表做匹配。
- 检查在sw-description中描述的所有组件是否真的在cpio归档中。
- 如果需要，修改分区。这包含UBI卷的大小调整，而不是MTD分区的大小调整。
  一个名为“data”的卷被用于在调整大小时保存和恢复数据。
- 执行预运行脚本
- 遍历所有镜像并调用相应的处理程序以便在目标上安装。
- 执行安装后脚本
- 如果在sw-description中指定了更改，则更新引导加载程序环境变量。
- 向操作人员报告状态(stdout)

有一个步骤失败，则会停止整个过程并报告错误。

运行SWUpdate从文件中获取镜像:

::

	        swupdate -i <filename>

带着嵌入式服务器启动:

::

	         swupdate -w "<web server options>"

web服务器主要的重要参数是"document-root"和"port"。

::

	         swupdate -w "--document-root ./www --port 8080"

嵌入式web服务器取自Mongoose项目。


检索所有选项列表:

::

        swupdate -h


这个完整使用随着代码交付的也没。当然，它们可以定制和替换。
网站使用AJAX与SWUpdate进行通信，并向操作人员显示更新的进度。

web服务器的默认端口是8080。你可以从如下网址连接到目标设备:

::

	http://<target_ip>:8080

如果它正常工作，则开始页面应该显示如下图所示。

.. image:: images/website.png

如果下载了正确的镜像，SWUpdate将开始处理接收到的镜像。
所有通知都被发送回浏览器。SWUpdate提供了一种机制，
可以将安装进度发送给接收方。实际上，SWUpdate接受
一个对象列表，这些对象在应用程序中注册了自身，
在调用notify()函数时就会通知它们。
这也允许自行编写处理程序通知上层错误条件或简单地返回状态。
这使得可以简单地添加一个自己的接收器，以实现以自定义的方式
显示结果：在LCD上显示(如果设备上有的话)，或者通过网络发送
回另一个设备。


发送回浏览器的通知示例如下图所示:

.. image:: images/webprogress.png

软件集合可以通过传递 `--select` 命令行选项来指定。
假设 `sw-description` 文件包含一个名为 `stable` 的集合，
加上 `alt` 的安装位置，则可以这样调用 `SWUpdate`

::

   swupdate --select stable,alt

命令行参数
-----------------------

+-------------+----------+--------------------------------------------+
|  Parameter  | Type     | 描述                                |
+=============+==========+============================================+
| -f <file>   | string   | 要使用的SWUpdate配置文件                   |
+-------------+----------+--------------------------------------------+
| -b <string> | string   | 只有当选上CONFIG_UBIATTACH时才有效，       |
|             |          | 它在SWUpdate搜索UBI卷时将MTDs列入黑名单。  |
|             |          | 示例:MTD0-1中的U-BOOT和环境变量            |
|             |          | **swupdate -b "0 1"**                      |
+-------------+----------+--------------------------------------------+
| -e <sel>    | string   | sel 的格式为 <software>,<mode>             |
|             |          | 它允许在sw-description文件中找到一个规则   |
|             |          | 的子集。有了这个选项就可以使用多重规则了   |
|             |          | 一种常见用法是在双拷贝模式下。例如:        |
|             |          | -e "stable, copy1"  ==> install on copy1   |
|             |          | -e "stable, copy2"  ==> install on copy2   |
+-------------+----------+--------------------------------------------+
| -h          |    -     | 使用帮助                                   |
+-------------+----------+--------------------------------------------+
| -k          | string   | 选中 CONFIG_SIGNED 时可用                  |
|             |          | 指定公钥文件                               |
+-------------+----------+--------------------------------------------+
| -l <level>  |    int   | 设置log级别                                |
+-------------+----------+--------------------------------------------+
| -L          |    -     | 将log输出到 syslog(local)                  |
+-------------+----------+--------------------------------------------+
| -i <file>   | string   | 使用本地.swu文件运行SWUpdate               |
+-------------+----------+--------------------------------------------+
| -n          |    -     | 在模拟(dry-run)模式下运行SWUpdate          |
+-------------+----------+--------------------------------------------+
| -N          | string   | 传入当前安装的软件版本。这将用于检查       |
|             |          | 新软件版本一起检查，禁止升级到旧版本。     |
|             |          | 版本号由4个数字组成:                       |
|             |          | major.minor.rev.build                      |
|             |          | 每个字段都要在0..65535的范围内             |
+-------------+----------+--------------------------------------------+
| -o <file>   | string   | 将流(SWU)保存到一个文件中                  |
+-------------+----------+--------------------------------------------+
| -v          |    -     | 激活详细的输出信息                         |
+-------------+----------+--------------------------------------------+
| -w <parms>  | string   | 启动内部webserver并将命令行字符串传递给它  |
+-------------+----------+--------------------------------------------+
| -u <parms>  | string   | 启动内部suricatta客户端守护进程，          |
|             |          | 并将命令行字符串传递给它                   |
|             |          | 详见suricatta的文档                        |
+-------------+----------+--------------------------------------------+
| -H          | string   | 设置板名和硬件版本                         |
| <board:rev> |          |                                            |
+-------------+----------+--------------------------------------------+
| -c          |    -     | 这个选项将检查 ``*.swu`` 文件的内部。      |
|             |          | 它确保sw-description中引用的文件是存在的。 |
|             |          | 使用方法: swupdate -c -i <file>            |
+-------------+----------+--------------------------------------------+
| -p          | string   | 执行安装后命令                             |
+-------------+----------+--------------------------------------------+
+-------------+----------+--------------------------------------------+
| -d <parms>  | string   | 选中 CONFIG_DOWNLOAD 时可用                |
|             |          | 启动内部下载程序客户端，                   |
|             |          | 并将命令行字符串传递给它。                 |
|             |          | 请参阅下载程序的内部命令行参数             |
+-------------+----------+--------------------------------------------+
| -u <url>    | string   | 这是提取新软件的URL。                      |
|             |          | URL是指向有效.swu镜像的链接                |
+-------------+----------+--------------------------------------------+
| -r <retries>| integer  | 下载失败前重试的次数。使用“-r 0”，则       |
|             |          | SWUpdate在加载到有效软件之前不会停止       |
+-------------+----------+--------------------------------------------+
| -t <timeout>| integer  | 判断下载连接丢失的超时时间                 |
+-------------+----------+--------------------------------------------+
| -a <usr:pwd>| string   | 发送用于基本身份验证的用户名和密码         |
+-------------+----------+--------------------------------------------+


systemd集成
-------------------

SWUpdate 具有可选的 systemd_ 支持，是由编译配置开关 ``CONFIG_SYSTEMD``
控制的。如果启用，SWUpdate将向systemd发送关于启动完成的信号，
并可以可选地使用systemd的socket-based activation功能。

一个systemd服务单元文件的示例 ``/etc/systemd/system/swupdate.service``
以suricatta守护进程模式启动SWUpdate，可能看起来像以下的样子：

::

	[Unit]
	Description=SWUpdate daemon
	Documentation=https://github.com/sbabic/swupdate
	Documentation=https://sbabic.github.io/swupdate

	[Service]
	Type=notify
	ExecStart=/usr/bin/swupdate -u '-t default -u http://localhost -i 25'

	[Install]
	WantedBy=multi-user.target

通过 ``systemctl start swupdate.service`` 进行启动, SWUpdate在
启动时(重新)创建套接字。为了使用socket-based activation，还必须附带一个
systemd套接字单元文件 ``/etc/systemd/system/swupdate.socket`` ：

::

	[Unit]
	Description=SWUpdate socket listener
	Documentation=https://github.com/sbabic/swupdate
	Documentation=https://sbabic.github.io/swupdate

	[Socket]
	ListenStream=/tmp/sockinstctrl
	ListenStream=/tmp/swupdateprog

	[Install]
	WantedBy=sockets.target


在 ``swupdate.socket`` 被启动后, systemd创建套接字文件，
并在SWupdate启动时将它们交给SWUpdate.
例如，当与 ``/tmp/swupdateprog`` 对话时，systemd启动
``swupdate.service`` 并移交套接字文件。
在以 ``systemctl start swupdate.service`` "常规"启动SWupdate时
也会传递Socket文件。


注意，两个 ``ListenStream=`` 指令中的套接字路径
必须与SWUpdate配置中的 ``CONFIG_SOCKET_CTRL_PATH``
和 ``CONFIG_SOCKET_PROGRESS_PATH`` 中的套接字路径匹配。
这里描述了缺省套接字路径配置。

.. _systemd: https://www.freedesktop.org/wiki/Software/systemd/


引导启动程序的修改
===========================

SWUpdate 包含了内核和一个根文件系统(镜像),这必须由一个引导加载程序
来启动。如果使用U-Boot, 可以实现以下机制:

- U-Boot检查是否需要进行软件更新(检查gpio、串行控制台等)。
- 脚本“altbootcmd”设置启动SWUpdate的规则
- 当需要SWUpdate时, U-boot运行脚本"altbootcmd"

更改U-Boot环境变量是安全的吗？是的，但是必须正确配置U-Boot。
Uboot支持双备份环境变量，这可以使得更新器件掉电是安全的。
板子的配置文件必须定义CONFIG_ENV_OFFSET_REDUND或CONFIG_ENV_ADDR_REDUND。
查阅U-Boot文档了解这些常量的作用以及如何使用它们。

还有一些可选的增强可以集成到U-boot中，以使系统更安全。
其中我会建议的最重要的一个，是添加启动技术支持到uboot中
(文档在uboot的docs路径下)。这讲允许U-Boot追踪对成功启动应用的尝试。
如果启动计数超过了限制，则可以自动启动SWupdate，以替代损坏了的软件。

GRUB默认情况下不像U-Boot那样支持环境变量的双副本。
这意味着，在环境块更新期间断电时，环境块有可能损坏。
为了最小化风险，我们没有直接修改原始环境块。
而是将变量写入临时文件，并在操作成功后调用rename指令。

构建一个单个的镜像
=======================

cpio由于其简单性而被用作容器。由此可以很简单地生成镜像。
描述镜像的文件(默认是"sw-description"，但是名称是可以配置的)
必须是cpio归档中的第一个文件。
要生成镜像，可以使用以下脚本:


::

	CONTAINER_VER="1.0"
	PRODUCT_NAME="my-software"
	FILES="sw-description image1.ubifs  \
	       image2.gz.u-boot uImage.bin myfile sdcard.img"
	for i in $FILES;do
		echo $i;done | cpio -ov -H crc >  ${PRODUCT_NAME}_${CONTAINER_VER}.swu


单个的子图像可以在cpio容器中按任意顺序放置，除了sw-description，它必须是第一个子镜像。
要检查生成的镜像，可以运行以下命令:

::

    swupdate -c -i my-software_1.0.swu


对复合镜像的支持
-------------------------

在Yocto中可以自动生成单个镜像。
meta-swupdate使用swupdate类扩展了类。
配方应该继承它，并添加自己的sw-description文件来生成镜像。
