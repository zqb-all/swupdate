=================================================
SWUpdate:使用默认解析器的语法和标记
=================================================

介绍
------------

SWUpdate使用库“libconfig”作为镜像描述的默认解析器。
但是，可以扩展SWUpdate并添加一个自己的解析器，
以支持不同于libconfig的语法和语言。
在examples目录中，有一个用Lua编写的，支持解析XML形式
描述文件的解析器。

使用默认解析器，则sw-description遵循libconfig手册中描述的语法规则。
请参阅http://www.hyperrealm.com/libconfig/libconfig_manual.html
以了解基本类型。
整个描述必须包含在sw-description文件中:
SWUpdate不允许使用#include指令。
下面的例子更好地解释了当前实现的标记:


::

	software =
	{
		version = "0.1.0";
		description = "Firmware update for XXXXX Project";

		hardware-compatibility: [ "1.0", "1.2", "1.3"];

		/* partitions tag is used to resize UBI partitions */
		partitions: ( /* UBI Volumes */
			{
				name = "rootfs";
				device = "mtd4";
			  	size = 104896512; /* in bytes */
			},
			{
				name = "data";
				device = "mtd5";
		  		size = 50448384; /* in bytes */
			}
		);


		images: (
			{
				filename = "rootfs.ubifs";
				volume = "rootfs";
			},
			{
				filename = "swupdate.ext3.gz.u-boot";
				volume = "fs_recovery";
			},
			{
				filename = "sdcard.ext3.gz";
				device = "/dev/mmcblk0p1";
				compressed = true;
			},
			{
				filename = "bootlogo.bmp";
				volume = "splash";
			},
			{
				filename = "uImage.bin";
				volume = "kernel";
			},
			{
				filename = "fpga.txt";
				type = "fpga";
			}
		);

		files: (
			{
				filename = "README";
				path = "/README";
				device = "/dev/mmcblk0p1";
				filesystem = "vfat"
			}
		);

		scripts: (
			{
				filename = "erase_at_end";
				type = "lua";
		 	},
			{
				filename = "display_info";
				type = "lua";
			}
		);

		bootenv: (
			{
				filename = "bootloader-env";
				type = "bootloader";
			},
			{
				name = "vram";
				value = "4M";
			},
			{
				name = "addfb";
				value = "setenv bootargs ${bootargs} omapfb.vram=1:2M,2:2M,3:2M omapdss.def_disp=lcd"
			}
		);
	}

第一个标签是“软件”。整个描述包含在这个标签中。
可以使用 `特定的板级设置`_ _对每个设备的设置进行分组。

处理配置的差异
----------------------------------

这个概念可以扩展到交付单个映像，在其中包含用于多个不同设备的发布。
每个设备都有自己的内核、dtb和根文件系统，或者它们可以共享某些部分。

目前，这是通过编写自己的解析器来管理的(并且已经在实际项目中使用)，
解析器在识别出软件当前运行在什么设备上之后，检查必须安装哪些镜像。
因为外部解析器可以用Lua编写，而且它是完全可定制的，
所以每个人都可以设置自己的规则。
对于这个特定的例子，sw-description是用XML格式编写的，
带有标识来标记每个设备对应的镜像。要运行它需要liblxp库。

::

	<?xml version="1.0" encoding="UTF-8"?>
	<software version="1.0">
	  <name>Update Image</name>
	  <version>1.0.0</version>
	  <description>Firmware for XXXXX Project</description>

	  <images>
	    <image device="firstdevice" version="0.9">
	      <stream name="dev1-uImage" type="ubivol" volume="kernel" />
	      <stream name="dev1.dtb" type="ubivol" volume="dtb" />
	      <stream name="dev1-rootfs.ubifs" type="ubivol" volume="rootfs"/>
	      <stream name="dev1-uboot-env" type="uboot" />
	      <stream name="raw_vfat" type="raw" dest="/dev/mmcblk0p4" />
	      <stream name="sdcard.lua" type="lua" />
	    </image>

	    <image device="seconddevice" version="0.9">
	      <stream name="dev2-uImage" type="ubivol" volume="kernel" />
	      <stream name="dev2.dtb" rev="0.9" type="ubivol" volume="dtb" />
	      <stream name="dev2-rootfs.ubifs" type="ubivol" volume="rootfs"/>
	    </image>
	  </images>
	</software>

支持本例子的解析器位于/examples目录中。
通过识别哪个是正在运行的设备，解析器返回一个表，
其中包含必须安装的镜像及其关联的处理程序。

读取交付的镜像时，SWUpdate将忽略解析器处理列表之外的所有镜像。
通过这种方式，可以使用单个交付镜像来更新多个设备。

默认解析器也支持多个设备。

::

    software =
    {
        version = "0.1.0";

        target-1 = {
                images: (
                        {
                                ...
                        }
                );
        };

        target-2 = {
                images: (
                        {
                                ...
                        }
                );
        };
    }

通过这种方式，可以使用单个镜像为你的所有设备提供软件。

默认情况下，硬件信息是从 `/etc/hwrevision` 文件中提取的。
文件应包含单行信息，格式如下::

  <boardname> <revision>

Where:

- `<revision>` 将用于与硬件兼容列表匹配

- `<boardname>` 可用于对板子的具体设置进行分组

.. _collections:

软件集合
--------------------

软件集合和操作模式可用于实现双拷贝策略。
最简单的情况是为固件映像定义两个安装位置，
并在调用 `SWUpdate` 时选择适当的镜像。

::

    software =
    {
            version = "0.1.0";

            stable = {
                    copy-1: {
                            images: (
                            {
                                    device = "/dev/mtd4"
                                    ...
                            }
                            );
                    }
                    copy-2: {
                            images: (
                            {
                                    device = "/dev/mtd5"
                                    ...
                            }
                            );
                    }
            };
    }

通过这种方式，可以指定 `copy-1` 安装到 `/dev/mtd4` ，
而 `copy-2` 安装到 `/dev/mtd5` 。
通过正确选择安装位置， `SWUpdate` 将更新另一个插槽中的固件。

具体镜像的选择方法超出了SWUpdate的范围内，
用户要负责调用 `SWUpdate` 并传入适当的设置。

查找文件元素的优先级
-----------------------------------------

SWUpdate根据以下优先级搜索sdw-description文件中的条目:

1. 尝试 <boardname>.<selection>.<mode>.<entry>
2. 尝试 <selection>.<mode>.<entry>
3. 尝试 <boardname>.<entry>
4. 尝试 <entry>

举一个例子。下面的sw-description描述了一组板子的发布。

::

    software =
    {
            version = "0.1.0";

            myboard = {
                stable = {
                    copy-1: {
                            images: (
                            {
                                    device = "/dev/mtd4"
                                    ...
                            }
                            );
                    }
                    copy-2: {
                            images: (
                            {
                                    device = "/dev/mtd5"
                                    ...
                            }
                            );
                    }
                }
            }

            stable = {
                copy-1: {
                      images: (
                          {
                               device = "/dev/mtd6"
                                    ...
                          }
                       );
                }
                copy-2: {
                       images: (
                       {
                               device = "/dev/mtd7"
                                    ...
                       }
                       );
                }
            }
    }

在 *myboard* 上运行时，SWUpdate会搜索并找到myboard.stable.copy1(2)。
当在其他板子上运行时，SWUpdate则无法找到一个与板子名字对应的条目，
那它就会退回到没有指定板子名字的版本。
这样就可以使用一个发布版本，适配拥有完全不同硬件的不同板子。
例如, `myboard` 可以是eMMC和ext4文件系统，而另一个设备可以是raw flash并安装
UBI文件系统。然而，它们都是同一版本的不同格式，可以在sw-description中一起描述。
重要的是，要理解SWUpdate在解析期间如何按优先级扫描条目。

使用链接
-----------

sw-description可能变得非常复杂。
让我们假设只有一个板子，但是存在多个硬件版本，它们在硬件上是不同的。
这些版本中有些可以统一处理，有些则需要特殊的部分。
一种方法(但不是唯一的方法!)是添加 *mode* 并使用 `-e stable,<rev number>` 做选择。

::

	software =
	{
		version = "0.1.0";

		myboard = {
	            stable = {

			hardware-compatibility: ["1.0", "1.2", "2.0", "1.§, "3.0", "3.1"];
			rev-1.0: {
				images: (
					...
				);
				scripts: (
					...
				);
			}
			rev-1.2: {
				hardware-compatibility: ["1.2"];
				images: (
					...
				);
				scripts: (
					...
				);
			}
			rev-2.0: {
				hardware-compatibility: ["2.0"];
				images: (
					...
				);
				scripts: (
                                   ...
				);
			}
			rev-1.3: {
				hardware-compatibility: ["1.3"];
				images: (
                                    ...
				);
				scripts: (
                                    ...
				);
			}

			rev-3.0:
			{
				hardware-compatibility: ["3.0"];
				images: (
					...
				);
				scripts: (
					...
				);
	                }
			rev-3.1:
			{
				hardware-compatibility: ["3.1"];
				images: (
					...
				);
				scripts: (
					...
				);
			}
		     }
	        }
	}


如果它们每个都需要一个单独的部分，那么这是一种方法。
尽管如此，更可能的情况时，不同的修订版本可以被当成一类，
例如，具有相同主要修订号的板子可能具有相同的安装说明。
在这个例子中，则可导出三个分组，rev1.X, rev2.X 和 rev3.X。
链接允许将部分分组在一起。当SWUpdate搜索组
(images、files、scripts、bootenv)时，如果发现“ref”，
它将用字符串的值替换树中的当前路径。这样，上面的例子可以这样写:

::

	software =
	{
                version = "0.1.0";

                myboard = {
	            stable = {

                        hardware-compatibility: ["1.0", "1.2", "2.0", "1.3, "3.0", "3.1"];
                        rev-1x: {
                                images: (
                                   ...
                                );
                                scripts: (
                                    ...
                                );
                        }
                        rev1.0 = {
                                ref = "#./rev-1x";
                        }
                        rev1.2 = {
                                ref = "#./rev-1x";
                        }
                        rev1.3 = {
                                ref = "#./rev-1x";
                        }
                        rev-2x: {
                                images: (
                                     ...
                                );
                                scripts: (
                                     ...
                                );
                        }
                        rev2.0 = {
                                ref = "#./rev-2x";
                        }

                        rev-3x: {
                                images: (
                                     ...
                                );
                                scripts: (
                                      ...
                                );
	                }
                        rev3.0 = {
                                ref = "#./rev-3x";
                        }
                        rev3.1 = {
                                ref = "#./rev-3x";
                        }
		     }
	        }
       }


这种链接可以是绝对的，也可以是相对的。关键字 *ref*  用于指示一个链接。
如果找到链接，SWUpdate将遍历树，并将当前路径替换为 "ref" 指向的字符串中的值。
用于链接的规则很简单：


       - 必须以字符 '#' 开头
       - "." 指向树中的当前层级，即 "ref" 的父级
       - ".." 指向树中的父级
       - "/" 在链接中用作字段分隔符

一个相对路径有许多前导 "../" 以从当前位置移动到树的高层级节点
在下面的例子中，rev40设置了一个链接到 "common", 在那可以找到 "images"。
这也是通过链接到父节点中的一个部分来设置的。
路径 `software.myboard.stable.common.images` 被替换为
`software.myboard.stable.trythis`

::

	software =
	{
	  version = {
		  ref = "#./commonversion";
	  }

	  hardware-compatibility = ["rev10", "rev11", "rev20"];

	  commonversion = "0.7-linked";

	pc:{
	  stable:{

	    common:{
		images =
		{
		  ref = "#./../trythis";
		}
	      };

	    trythis:(
		{
		filename = "rootfs1.ext4";
		device = "/dev/mmcblk0p8";
		type = "raw";
		} ,
		{
		filename = "rootfs5.ext4";
		device = "/dev/mmcblk0p7";
		type = "raw";
		}
	      );
	    pdm3rev10:
	      {
	      images:(
		  {
		  filename = "rootfs.ext3"; device = "/dev/mmcblk0p2";}
		);
	      uboot:(
		  { name = "bootpart";
		  value = "0:2";}
		);
	      };
	      pdm3rev11 =
	      {
		ref = "#./pdm3rev10";
	      }
	      pdm3rev20 =
	      {
		ref = "#./pdm3rev10";
	      }
	      pdm3rev40 =
	      {
		ref = "#./common";
	      }
	    };
	  };
	}

可以通过链接重定向sw-description中的每个条目，就像上面示例中的 "version" 属性那样。

硬件兼容性
----------------------

硬件兼容性: [ "major.minor", "major.minor", ... ]

它列出了与此软件镜像兼容的硬件修订版本。

例子:

	hardware-compatibility: [ "1.0", "1.2", "1.3"];


这意味着该软件可以兼容硬件修订版本1.0, 1.2 和 1.3,但不能兼容1.1
和其他未在此明确列出的版本。
如何找到正在运行SWUpdate的板子的修订版本，是另一件事情了。
这里并没有假设如何获得修订版本（可以通过GPIOs,EEPROM等),
每个项目都可以自由选择最合适的方式。
在启动SWUpdate之前，结果必须写入文件/etc/hwrevision(如果配置中
指定了另一个文件，则必须写入对应的文件)。

partitions : UBI 布局
-----------------------

此标记允许更改UBI卷的布局。
请注意，此处不涉及MTDs，它们是由设备树配置的，
或者直接在内核中以另一种方式配置的。


::

	partitions: (
		{
			name = <volume name>;
			size = <size in bytes>;
			device = <MTD device>;
		},
	);

所有字段都是强制的。SWUpdate搜索所选名称的卷并调整大小，
如果不存在具有给定名称的卷，则创建新卷。
在后一种情况下，它是在连接到"device"所指定MTD设备的UBI设备上创建的。
"device"可以以数字(如 "mtd4")或名字(及MTD设备的名字，如 "ubi_partition")
的方式给出。UBI设备的连接是自动进行的。

images
------

标签 "images" 收集安装到系统中的映像。
语法是:

::

	images: (
		{
			filename[mandatory] = <Name in CPIO Archive>;
			volume[optional] = <destination volume>;
			device[optional] = <destination volume>;
			mtdname[optional] = <destination mtd name>;
			type[optional] = <handler>;
			/* optionally, the image can be copied at a specific offset */
			offset[optional] = <offset>;
			/* optionally, the image can be compressed if it is in raw mode */
			compressed;
		},
		/* Next Image */
		.....
	);

*volume* 仅用于将镜像安装到UBI卷中。 *volume* 和 *device* 不能同时使用。
如果设置了device,则会自动选中裸数据处理程序(raw handler)。

以下时一个更新UBI卷的例子:


::

		{
			filename = "core-image-base.ubifs";
			volume = "rootfs";
		}

要以裸数据形式更新体格镜像，语法如下：


::

		{
			filename = "core-image-base.ext3";
			device = "/dev/mmcblk0p1";
		}

要将镜像写入到一个指定偏移处，语法如下：


::

		{
			filename = "u-boot.bin";
			device = "/dev/mmcblk0p1";
			offset = "16K";
		}

偏移量可处理以下乘法后缀:K=1024和M=1024*1024。

但是，在裸数据模式下写flash必须以一种特殊的方式进行管理。
Flash在写入之前必须先擦除，并且写入NAND时必须处理坏块和ECC错误。
因此，必须选择处理程序"flash":

例如，要将内核复制到NAND闪存的MTD7中:

::

		{
			filename = "uImage";
			device = "mtd7";
			type = "flash";
		}


*filename* 是必须的。它是由流提取的文件的名称。
*volume* 仅在UBI卷中是强制性的。它不应该在其他情况下使用。

另外，对于处理程序 "flash"，可以指定 *mtdname* 来代替设备名称:

::

		{
			filename = "uImage";
			mtdname = "kernel";
			type = "flash";
		}


Files
-----

可以复制单个文件而不是完整镜像。
这不是首选的方法，但是可以用于调试或特殊目的。

::

	files: (
		{
			filename = <Name in CPIO Archive>;
			path = <path in filesystem>;
			device[optional] = <device node >;
			filesystem[optional] = <filesystem for mount>;
			properties[optional] = {create-destination = "true";}
		}
	);

"files" 部分中的条目会作为单个文件进行管理。
"filename" 和 "path" 属性是必须的。
属性 "device" 和 "filesystem" 是可选的;
它们用于告诉SWUpdate，在将"filename"拷贝到"path"之前
先挂载设备(以给定的文件系统类型进行挂载，如 "ext4")。
如果没有指定"device"和"filesystem"，
则"filename"会被拷贝到当前根文件系统的"path"。

一般来说，如果目标路径不存在，swupdate不会复制文件。
可以使用特殊属性"create-destination"更改此行为。


Scripts
-------

脚本按照它们被放入sw-description文件的顺序运行。
脚本的结果由SWUpdate进行评估，如果结果是<> 0，则停止更新并报错。

它们在执行之前会被复制到一个临时目录中，
并且它们的名字在同一个cpio归档中必须是惟一的。

如果没有给出类型，SWUpdate默认为 "lua"。


Lua
...

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "lua";
	 	},
	);

Lua脚本使用内部解释器运行。

它们必须具有下列函数中的至少一个:

::

	function preinst()

SWUpdate扫描所有脚本并检查preinst函数。在安装镜像之前调用它。

::

	function postinst()

SWUpdate扫描所有脚本并检查postinst函数。它是在安装镜像之后调用的。

shellscript
...........

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "shellscript";
		},
	);


Shell脚本通过system命令调用。
SWUpdate扫描所有脚本，并在安装镜像之前和之后调用它们。
SWUpdate将'preinst'或'postinst'作为脚本的第一个参数传递。
如果定义了data属性，它的值将作为最后一个参数传递给脚本。

preinstall
..........

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "preinstall";
		},
	);

preinstall 是通过system命令调用的shell脚本。
SWUpdate扫描所有脚本并在安装映像之前调用它们。
如果定义了data属性，它的值将作为最后一个参数传递给脚本。

postinstall
...........

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "postinstall";
		},
	);

postinstall 是通过system命令调用的shell脚本。
SWUpdate扫描所有脚本，并在安装镜像后调用它们。
如果定义了data属性，它的值将作为最后一个参数传递给脚本。

bootloader
----------

有两种方法可以更新引导加载程序(当前支持U-Boot、GRUB和EFI Boot Guard)
的环境变量。
第一种方法是添加一个包含要更改的变量列表的文件，
并将“bootloader”设置为镜像的类型。
这将通知SWUpdate调用引导加载程序处理程序来处理文件
(需要在配置中启用引导加载程序处理程序)。
对于所有受支持的引导加载程序，都有一个引导加载程序处理程序。
必须从 `menuconfig` 的引导加载程序选择菜单中选择适当的引导加载程序。

::

	bootenv: (
		{
			filename = "bootloader-env";
			type = "bootloader";
		},
	)

文件的格式在U-boot文档中有描述。每一行都是如下格式

::

	<name of variable>	<value>

如果值缺失，则变量将被去掉。
在当前实现中，GRUB和EFI Boot Guard 的环境变量修改也继承了上述文件格式。

第二种方法是在组设置中定义需要更改的变量:

::

	bootenv: (
		{
			name = <Variable name>;
			value = <Variable value>;
		},
	)

SWUpdate将在内部生成一个脚本，该脚本将传递给
引导加载程序处理程序，用于调整环境变量。

为了向后兼容以前构建的 `.swu`  镜像，"uboot" 组名仍然作为别名支持。
但是，它实际上已经被弃用了，不建议继续使用它。


特定的板级设置
-----------------------

每个设置都可以放在与板名匹配的自定义标记下。
此机制可用于以板卡特有的方式覆盖特定设置。

假设硬件信息文件 `/etc/hwrevision` 包含以下条目::

  my-board 0.1.0

以及以下描述::

	software =
	{
	        version = "0.1.0";

	        my-board = {
	                bootenv: (
	                {
	                        name = "bootpart";
	                        value = "0:2";
	                }
	                );
	        };

	        bootenv: (
	        {
	                name = "bootpart";
	                value = "0:1";
	        }
	        );
	}

SWUpdate将在这个板子的引导加载程序环境中将 `bootpart` 设置为 `0:2` 。
对于所有其他板子， `bootpart` 将被设置为 `0:1` 。
特定于板子的设置优先于默认作用域的设置。

软件集合和操作模式
----------------------------------------

软件集合和操作模式扩展了描述文件语法，
以提供对之前介绍的所有配置标记的叠加分组。
这种机制类似于 `特定的板级设置`_ ,可用于实现双拷贝策略，
或者用单个更新文件内同时交付稳定和不稳定版本的镜像。


该机制使用放置在 `software` 标签范围内的自定义用户定义标签。
标签不能使用以下名字: `version`, `hardware-compatibility`,
`uboot`, `bootenv`, `files`, `scripts`, `partitions`, `images`

示例描述文件:

::

	software =
	{
	        version = "0.1";

	        hardware-compatibility = [ "revA" ];

	        /* differentiate running image modes/sets */
	        stable:
	        {
	                main:
	                {
	                        images: (
	                        {
	                                filename = "rootfs.ext3";
	                                device = "/dev/mmcblk0p2";
	                        }
	                        );

	                        bootenv: (
	                        {
	                                name = "bootpart";
	                                value = "0:2";
	                        }
	                        );
	                };
	                alt:
	                {
	                        images: (
	                        {
	                                filename = "rootfs.ext3";
	                                device = "/dev/mmcblk0p1";
	                        }
	                        );

	                        bootenv: (
	                        {
	                                name = "bootpart";
	                                value = "0:1";
	                        }
	                        );
	                };

	        };
	}



这个配置描述了一个名为 `stable` 的软件集合。
并为这个集合指定了两个不同的镜像安装位置: `/dev/mmcblk0p1` 和
`/dev/mmcblk0p2` 分别用于 `main` 模式和 `alt` 模式。

该特性可以通过显式指定集合和模式来实现双拷贝策略。

检查已安装软件的版本
--------------------------------------

SWUpdate支持可选地验证子镜像是否已经被安装了，
如果要安装的版本完全相同，则可以跳过它的安装。
这在安装某些高风险镜像或需要加速升级过程的情况下是非常有用的。

一种情况是需要更新引导加载程序。在大多数情况下，
不需要升级引导加载程序，但是实践表明，在某些情况下，
确实有必要升级 - 项目经理应该承担这个风险。
经过如此，始终将引导加载程序镜像作为.swu文件的一部分是更好的，
这样可以在单个文件中获得设备的整个发行版，但是设备应该仅在必要时安装它。

SWUpdate搜索包含已安装映像的所有版本信息的文件(默认位置是/etc/sw-versions)。
这个文件必须在运行SWUpdate之前生成。

文件必须包含成对的信息，即镜像名称和版本:

::

	<name of component>	<version>

版本是一个字符串，可以有任何值。例如:

::

        bootloader              2015.01-rc3-00456-gd4978d
        kernel                  3.17.0-00215-g2e876af

在sw-description中，可选属性 "name"、"version"
和"install-if-different"提供了连接。
name和version将用于与版本文件中的数据进行比较。
install-if-different则是一个布尔值，用于对此镜像启用版本检查。
这样就可以只对要安装的镜像们的一个子集进行版本检查。

嵌入脚本
---------------

可以将脚本嵌入到sw-description中。这在许多情况下非常有用，
因为一些参数只有在目标上实际运行时知道。
脚本是全局的，面向所有部分，但是它可以包含几个函数，
这些函数可以针对sw-description文件中的每个条目。

这些属性用于嵌入脚本:

::

		embedded-script = "<Lua code">

必须考虑到解析器已经在运行，双引号的使用可能会干扰解析器。
因此，脚本中的每个双引号都必须转义。

这意味着像这样的一个简单的Lua代码:

::

        print ("Test")

修改改成这样:

::

        print (\"Test\")

不然解析器会认为脚本已经关闭，并产生一个错误。
有关如何使用它的示例，请参见示例目录。
文件或镜像中的任何条目都可以触发脚本中的一个函数。
"hook" 属性告诉解析器加载脚本并搜索钩子属性指向的函数。例如:

::

		files: (
			{
				filename = "examples.tar";
				type = "archive";
				path = "/tmp/test";
				hook = "set_version";
				preserve-attributes = true;
			}
		);

在解析条目之后，解析器运行hook所指向的Lua函数。
如果Lua未被激活，解析器将引发一个错误，
因为必须解析带有嵌入脚本的sw-description，但解释器不可用。
每个Lua函数接收一个带有当前条目设置的表作为参数。
Lua钩子的格式是:

::

        function lua_hook(image)

参数image是一个表，其关键字是有效属性的列表。
如果一个属性包含了"-"，则会被替换为"_"，因为Lua中不能使用 "-"。
这意味着，如下例子：

::

        install-if-different ==> install_if_different
        install-directly     ==> install_directly

可以在Lua脚本中更改属性，并在返回时接管值。
Lua函数必须返回2个值:

        - 一个布尔值，指示解析是否正确
        - 镜像表或nil以表示应该跳过该镜像

例子:

::

        function set_version(image)
	        print (\"RECOVERY_STATUS.RUN: \".. swupdate.RECOVERY_STATUS.RUN)
                for k,l in pairs(image) do
                        swupdate.trace(\"image[\" .. tostring(k) .. \"] = \" .. tostring(l))
                end
	        image.version = \"1.0\"
        	image.install_if_different = true
        	return true, image
        end

该示例为已安装镜像设置了一个版本。
通常，这是在运行时从目标读取数据检测到的。

.. _sw-description-attribute-reference:

属性参考
-------------------

在sw-description中有4个主要部分:

- images: 条目是镜像，SWUpdate对它们一无所知。
- files: 条目是文件，SWUpdate需要一个用于它们的文件系统。
  这通常用于从tar-ball展开或更新单个文件。
- scripts: 所有条目都被视为可执行文件，它们将被运行两次(作为安装前和安装后脚本)。
- bootenv:条目是引导加载程序环境变量名及其值的键值对。


.. tabularcolumns:: |p{1.5cm}|p{1.5cm}|p{1.5cm}|L|
.. table:: Attributes in sw-description


   +-------------+----------+------------+---------------------------------------+
   |  名字       |  类型    | 应用于     |  描述                                 |
   +=============+==========+============+=======================================+
   | filename    | string   | images     | 在cpio存档中找到的文件名。            |
   |             |          | files      |                                       |
   |             |          | scripts    |                                       |
   +-------------+----------+------------+---------------------------------------+
   | volume      | string   | images     | 仅在 type = "ubivol"时使用。          |
   |             |          |            | 指明镜像将安装到哪个UBI卷。           |
   +-------------+----------+------------+---------------------------------------+
   | ubipartition| string   | images     | 仅在 type = "ubivol"时使用。          |
   |             |          |            | 要创建或调整大小的UBI卷。             |
   +-------------+----------+------------+---------------------------------------+
   | device      | string   | images     | 在/dev下可找到的设备节点，或者是到它的|
   |             |          | files      | 符号链接。 可以指定为绝对路径，或/dev |
   |             |          |            | 下的名字。例如，如果/dev/mtd-dev是一个|
   |             |          |            | 指向/dev/mtd3的链接，则 "mtd3",       |
   |             |          |            | "mtd-dtb","/dev/mtd3"和"/dev/mtd-dtb" |
   |             |          |            | 均是有效的名字。                      |
   |             |          |            | 用法取决于具体处理程序。              |
   |             |          |            | 对于文件，用于指明哪个设备用于挂载    |
   |             |          |            | "filesystem"，如果未指定，则使用当前的|
   |             |          |            | 根文件系统。                          |
   +-------------+----------+------------+---------------------------------------+
   | filesystem  | string   | files      | 指示文件安装位置的文件系统类型。      |
   |             |          |            | 仅在设置了"device"属性时使用。        |
   +-------------+----------+------------+---------------------------------------+
   | path        | string   | files      | 用于文件:指示用于安装文件的路径       |
   |             |          |            | (绝对路径)。如果设置了"device" 和     |
   |             |          |            | "filesystem", Swupdate将在以指定文件  |
   |             |          |            | 系统类型挂载设备后再安装文件。        |
   |             |          |            | (路径总是相对于挂载点而言的)          |
   +-------------+----------+------------+---------------------------------------+
   | preserve-\  | bool     | files      | 标记，用于控制从归档文件解压文件时    |
   | attributes  |          |            | 是否保留下列属性                      |
   |             |          |            | (当然，前提是目标文件系统支持它们):   |
   |             |          |            | timestamp, uid/gid (numeric), perms,  |
   |             |          |            | file attributes, extended attributes  |
   +-------------+----------+------------+---------------------------------------+
   | type        | string   | images     | 处理程序的字符串标识符，              |
   |             |          | files      | 它是由处理程序在注册自身时设置的。    |
   |             |          | scripts    | 例如: "ubivol", "raw", "rawfile"      |
   +-------------+----------+------------+---------------------------------------+
   | compressed  | bool     | images     | 标记，用于指示"filename"是zlib压缩的，|
   |             |          | files      | 在安装之前必须先解压                  |
   +-------------+----------+------------+---------------------------------------+
   | installed-\ | bool     | images     | 标志，用于指示镜像需流式更新到目标中, |
   | directly    |          |            | 不需要任何临时副本。                  |
   |             |          |            | 并非所有处理程序都支持流式更新。      |
   +-------------+----------+------------+---------------------------------------+
   | name        | string   | bootenv    | 要设置的引导程序环境变量的名字。      |
   +-------------+----------+------------+---------------------------------------+
   | value       | string   | bootenv    | 要赋给引导加载程序环境变量的值。      |
   +-------------+----------+------------+---------------------------------------+
   | name        | string   | images     | 标识sw-component的名称，它可以是任何  |
   |             |          | files      | 字符串，将与sw-versions中的条目做比较 |
   +-------------+----------+------------+---------------------------------------+
   | version     | string   | images     | sw-component的版本，可以是任何字符串，|
   |             |          | files      | 将与sw-version中的条目做比较          |
   +-------------+----------+------------+---------------------------------------+
   | description | string   |            | swupdate归档文件的用户友好的描述      |
   |             |          |            | (可使用任意字符串)                    |
   +-------------+----------+------------+---------------------------------------+
   | install-if\ | bool     | images     | 标志                                  |
   | -different  |          | files      | 如果设置了，名字和版本会于版本文件中  |
   |             |          |            | 的条目做比较。                        |
   +-------------+----------+------------+---------------------------------------+
   | encrypted   | bool     | images     | 标志                                  |
   |             |          | files      | 如果设置了, 文件会被加密并必须先解密后|
   |             |          | scripts    | 安装。                                |
   +-------------+----------+------------+---------------------------------------+
   | data        | string   | images     | 用于将任意数据传递给处理程序。        |
   |             |          | files      |                                       |
   |             |          | scripts    |                                       |
   +-------------+----------+------------+---------------------------------------+
   | sha256      | string   | images     | 镜像、文件或脚本的sha256哈希值。      |
   |             |          | files      | 用于签名镜像的校验。                  |
   |             |          | scripts    |                                       |
   +-------------+----------+------------+---------------------------------------+
   | embedded-\  | string   |            | 嵌入sw-description文件中的Lua代码。   |
   | script      |          |            |                                       |
   +-------------+----------+------------+---------------------------------------+
   | offset      | string   | images     | 可选的目的位置的偏移量。              |
   +-------------+----------+------------+---------------------------------------+
   | hook        | string   | images     | 解析条目时要调用的函数(Lua)的名称。   |
   |             |          | files      |                                       |
   +-------------+----------+------------+---------------------------------------+
   | mtdname     | string   | images     | 要更新的MTD的名称。仅被flash处理程序  |
   |             |          |            | 用来代替具体的设备节点名，以识别要    |
   |             |          |            | 更新的MTD。                           |
   +-------------+----------+------------+---------------------------------------+
