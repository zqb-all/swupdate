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
可以使用 `Board specific settings`_ _对每个设备的设置进行分组。

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

Using links
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

hardware-compatibility
----------------------

hardware-compatibility: [ "major.minor", "major.minor", ... ]

It lists the hardware revisions that are compatible with this software image.

Example:

	hardware-compatibility: [ "1.0", "1.2", "1.3"];

This means that the software is compatible with HW-Revisions
1.0, 1.2 and 1.3, but not for 1.1 or other version not explicitly
listed here.
It is then duty of the single project to find which is the
revision of the board where SWUpdate is running. There is no
assumption how the revision can be obtained (GPIOs, EEPROM,..)
and each project is free to select the way most appropriate.
The result must be written in the file /etc/hwrevision (or in
another file if specified as configuration option) before
SWUpdate is started.

partitions : UBI layout
-----------------------

This tag allows to change the layout of UBI volumes.
Please take care that MTDs are not touched and they are
configured by the Device Tree or in another way directly
in kernel.


::

	partitions: (
		{
			name = <volume name>;
			size = <size in bytes>;
			device = <MTD device>;
		},
	);

All fields are mandatory. SWUpdate searches for a volume of the
selected name and adjusts the size, or creates a new volume if
no volume with the given name exists. In the latter case, it is
created on the UBI device attached to the MTD device given by
"device". "device" can be given by number (e.g. "mtd4") or by name
(the name of the MTD device, e.g. "ubi_partition"). The UBI device
is attached automatically.

images
------

The tag "images" collects the image that are installed to the system.
The syntax is:

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

*volume* is only used to install the image in a UBI volume. *volume* and
*device* cannot be used at the same time. If device is set,
the raw handler is automatically selected.

The following example is to update a UBI volume:


::

		{
			filename = "core-image-base.ubifs";
			volume = "rootfs";
		}


To update an image in raw mode, the syntax is:


::

		{
			filename = "core-image-base.ext3";
			device = "/dev/mmcblk0p1";
		}

To flash an image at a specific offset, the syntax is:


::

		{
			filename = "u-boot.bin";
			device = "/dev/mmcblk0p1";
			offset = "16K";
		}

The offset handles the following multiplicative suffixes: K=1024 and M=1024*1024.

However, writing to flash in raw mode must be managed in a special
way. Flashes must be erased before copying, and writing into NAND
must take care of bad blocks and ECC errors. For this reasons, the
handler "flash" must be selected:

For example, to copy the kernel into the MTD7 of a NAND flash:

::

		{
			filename = "uImage";
			device = "mtd7";
			type = "flash";
		}

The *filename* is mandatory. It is the Name of the file extracted by the stream.
*volume* is only mandatory in case of UBI volumes. It should be not used
in other cases.

Alternatively, for the handler “flash”, the *mtdname* can be specified, instead of the device name:

::

		{
			filename = "uImage";
			mtdname = "kernel";
			type = "flash";
		}


Files
-----

It is possible to copy single files instead of images.
This is not the preferred way, but it can be used for
debugging or special purposes.

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

Entries in "files" section are managed as single files. The attributes
"filename" and "path" are mandatory. Attributes "device" and "filesystem" are
optional; they tell SWUpdate to mount device (of the given filesystem type,
e.g. "ext4") before copying "filename" to "path". Without "device" and
"filesystem", the "filename" will be copied to "path" in the current rootfs.

As a general rule, swupdate doesn't copy out a file if the destination path
doesn't exists. This behavior could be changed using the special property
"create-destination".

Scripts
-------

Scripts runs in the order they are put into the sw-description file.
The result of a script is valuated by SWUpdate, that stops the update
with an error if the result is <> 0.

They are copied into a temporary directory before execution and their name must
be unique inside the same cpio archive.

If no type is given, SWUpdate default to "lua".

Lua
...

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "lua";
	 	},
	);


Lua scripts are run using the internal interpreter.

They must have at least one of the following functions:

::

	function preinst()

SWUpdate scans for all scripts and check for a preinst function. It is
called before installing the images.


::

	function postinst()

SWUpdate scans for all scripts and check for a postinst function. It is
called after installing the images.

shellscript
...........

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "shellscript";
		},
	);

Shell scripts are called via system command.
SWUpdate scans for all scripts and calls them before and after installing
the images. SWUpdate passes 'preinst' or 'postinst' as first argument to
the script.
If the data attribute is defined, its value is passed as the last argument(s)
to the script.

preinstall
..........

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "preinstall";
		},
	);

preinstall are shell scripts and called via system command.
SWUpdate scans for all scripts and calls them before installing the images.
If the data attribute is defined, its value is passed as the last argument(s)
to the script.

postinstall
...........

::

	scripts: (
		{
			filename = <Name in CPIO Archive>;
			type = "postinstall";
		},
	);

postinstall are shell scripts and called via system command.
SWUpdate scans for all scripts and calls them after installing the images.
If the data attribute is defined, its value is passed as the last argument(s)
to the script.

bootloader
----------

There are two ways to update the bootloader (currently U-Boot, GRUB, and
EFI Boot Guard) environment. First way is to add a file with the list of
variables to be changed and setting "bootloader" as type of the image. This
informs SWUpdate to call the bootloader handler to manage the file
(requires enabling bootloader handler in configuration). There is one
bootloader handler for all supported bootloaders. The appropriate bootloader
must be chosen from the bootloader selection menu in `menuconfig`.

::

	bootenv: (
		{
			filename = "bootloader-env";
			type = "bootloader";
		},
	)

The format of the file is described in U-boot documentation. Each line
is in the format

::

	<name of variable>	<value>

if value is missing, the variable is unset.

In the current implementation, the above file format was inherited for
GRUB and EFI Boot Guard environment modification as well.

The second way is to define in a group setting the variables
that must be changed:

::

	bootenv: (
		{
			name = <Variable name>;
			value = <Variable value>;
		},
	)

SWUpdate will internally generate a script that will be passed to the
bootloader handler for adjusting the environment.

For backward compatibility with previously built `.swu` images, the
"uboot" group name is still supported as an alias. However, its usage
is deprecated.


Board specific settings
-----------------------

Each setting can be placed under a custom tag matching the board
name. This mechanism can be used to override particular setting in
board specific fashion.

Assuming that the hardware information file `/etc/hwrevision` contains
the following entry::

  my-board 0.1.0

and the following description::

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

SWUpdate will set `bootpart` to `0:2` in bootloader's environment for this
board. For all other boards, `bootpart` will be set to `0:1`. Board
specific settings take precedence over default scoped settings.


Software collections and operation modes
----------------------------------------

Software collections and operations modes extend the description file
syntax to provide an overlay grouping all previous configuration
tags. The mechanism is similar to `Board specific settings`_ and can
be used for implementing a dual copy strategy or delivering both
stable and unstable images within a single update file.

The mechanism uses a custom user-defined tags placed within `software`
scope. The tag names must not be any of: `version`,
`hardware-compatibility`, `uboot`, `bootenv`, `files`, `scripts`, `partitions`,
`images`

An example description file:

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

The configuration describes a single software collection named
`stable`. Two distinct image locations are specified for this
collection: `/dev/mmcblk0p1` and `/dev/mmcblk0p2` for `main` mode and
`alt` mode respectively.

This feature can be used to implement a dual copy strategy by
specifying the collection and mode explicitly.

Checking version of installed software
--------------------------------------

SWUpdate can optionally verify if a sub-image is already installed
and, if the version to be installed is exactly the same, it can skip
to install it. This is very useful in case some high risky image should
be installed or to speed up the upgrade process.
One case is if the bootloader needs to be updated. In most time, there
is no need to upgrade the bootloader, but practice showed that there are
some cases where an upgrade is strictly required - the project manager
should take the risk. However, it is nicer to have always the bootloader image
as part of the .swu file, allowing to get the whole distro for the
device in a single file, but the device should install it just when needed.

SWUpdate searches for a file (/etc/sw-versions is the default location)
containing all versions of the installed images. This must be generated
before running SWUpdate.
The file must contains pairs with the name of image and his version, as:

::

	<name of component>	<version>

Version is a string and can have any value. For example:

::

        bootloader              2015.01-rc3-00456-gd4978d
        kernel                  3.17.0-00215-g2e876af

In sw-description, the optional attributes "name", "version" and
"install-if-different" provide the connection. Name and version are then
compared with the data in the versions file. install-if-different is a
boolean that enables the check for this image. It is then possible to
check the version just for a subset of the images to be installed.


Embedded Script
---------------

It is possible to embed a script inside sw-description. This is useful in a lot
of conditions where some parameters are known just by the target at runtime. The
script is global to all sections, but it can contain several functions that can be specific
for each entry in the sw-description file.

These attributes are used for an embedded-script:

::

		embedded-script = "<Lua code">

It must be taken into account that the parser has already run and usage of double quotes can
interfere with the parser. For this reason, each double quote in the script must be escaped.

That means a simple Lua code as:

::

        print ("Test")

must be changed to:

::

        print (\"Test\")

If not, the parser thinks to have the closure of the script and this generates an error. 
See the examples directory for examples how to use it.
Any entry in files or images can trigger one function in the script. The "hook" attribute
tells the parser to load the script and to search for the function pointed to by the hook
attribute. For example:

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

After the entry is parsed, the parser runs the Lua function pointed to by hook. If Lua is not
activated, the parser raises an error because a sw-description with an embedded script must
be parsed, but the interpreter is not available.

Each Lua function receives as parameter a table with the setup for the current entry. A hook
in Lua is in the format:

::

        function lua_hook(image)

image is a table where the keys are the list of available attributes. If an attribute contains
a "-", it is replaced with "_", because "-" cannot be used in Lua. This means, for example, that:

::

        install-if-different ==> install_if_different
        install-directly     ==> install_directly

Attributes can be changed in the Lua script and values are taken over on return.
The Lua function must return 2 values:

        - a boolean, to indicate whether the parsing was correct
        - the image table or nil to indicate that the image should be skipped

Example:

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


The example sets a version for the installed image. Generally, this is detected at runtime
reading from the target.

.. _sw-description-attribute-reference:

Attribute reference
-------------------

There are 4 main sections inside sw-description:

- images: entries are images and SWUpdate has no knowledge
  about them.
- files: entries are files, and SWUpdate needs a filesystem for them.
  This is generally used to expand from a tar-ball or to update
  single files.
- scripts: all entries are treated as executables, and they will
  be run twice (as pre- and post- install scripts).
- bootenv: entries are pair with bootloader environment variable name and its
  value.


.. tabularcolumns:: |p{1.5cm}|p{1.5cm}|p{1.5cm}|L|
.. table:: Attributes in sw-description


   +-------------+----------+------------+---------------------------------------+
   |  Name       |  Type    | Applies to |  Description                          |
   +=============+==========+============+=======================================+
   | filename    | string   | images     |  filename as found in the cpio archive|
   |             |          | files      |                                       |
   |             |          | scripts    |                                       |
   +-------------+----------+------------+---------------------------------------+
   | volume      | string   | images     | Just if type = "ubivol". UBI volume   |
   |             |          |            | where image must be installed.        |
   +-------------+----------+------------+---------------------------------------+
   | ubipartition| string   | images     | Just if type = "ubivol". Volume to be |
   |             |          |            | created or adjusted with a new size   |
   +-------------+----------+------------+---------------------------------------+
   | device      | string   | images     | devicenode as found in /dev or a      |
   |             |          | files      | symlink to it. Can be specified as    |
   |             |          |            | absolute path or a name in /dev folder|
   |             |          |            | For example if /dev/mtd-dtb is a link |
   |             |          |            | to /dev/mtd3 "mtd3", "mtd-dtb",       |
   |             |          |            | "/dev/mtd3" and "/dev/mtd-dtb" are    |
   |             |          |            | valid names.                          |
   |             |          |            | Usage depends on handler.             |
   |             |          |            | For files, it indicates on which      |
   |             |          |            | device the "filesystem" must be       |
   |             |          |            | mounted. If not specified, the current|
   |             |          |            | rootfs will be used.                  |
   +-------------+----------+------------+---------------------------------------+
   | filesystem  | string   | files      | indicates the filesystem type where   |
   |             |          |            | the file must be installed. Only      |
   |             |          |            | used if "device" attribute is set.    |
   +-------------+----------+------------+---------------------------------------+
   | path        | string   | files      | For files: indicates the path         |
   |             |          |            | (absolute) where the file must be     |
   |             |          |            | installed. If "device" and            |
   |             |          |            | "filesystem" are set,                 |
   |             |          |            | SWUpdate will install the             |
   |             |          |            | file after mounting "device" with     |
   |             |          |            | "filesystem" type. (path is always    |
   |             |          |            | relative to the mount point.)         |
   +-------------+----------+------------+---------------------------------------+
   | preserve-\  | bool     | files      | flag to control whether the following |
   | attributes  |          |            | attributes will be preserved when     |
   |             |          |            | files are unpacked from an archive    |
   |             |          |            | (assuming destination filesystem      |
   |             |          |            | supports them, of course):            |
   |             |          |            | timestamp, uid/gid (numeric), perms,  |
   |             |          |            | file attributes, extended attributes  |
   +-------------+----------+------------+---------------------------------------+
   | type        | string   | images     | string identifier for the handler,    |
   |             |          | files      | as it is set by the handler when it   |
   |             |          | scripts    | regitsters itself.                    |
   |             |          |            | Example: "ubivol", "raw", "rawfile",  |
   +-------------+----------+------------+---------------------------------------+
   | compressed  | bool     | images     | flag to indicate that "filename" is   |
   |             |          | files      | zlib-compressed and must be           |
   |             |          |            | decompressed before being installed   |
   +-------------+----------+------------+---------------------------------------+
   | installed-\ | bool     | images     | flag to indicate that image is        |
   | directly    |          |            | streamed into the target without any  |
   |             |          |            | temporary copy. Not all handlers      |
   |             |          |            | support streaming.                    |
   +-------------+----------+------------+---------------------------------------+
   | name        | string   | bootenv    | name of the bootloader variable to be |
   |             |          |            | set.                                  |
   +-------------+----------+------------+---------------------------------------+
   | value       | string   | bootenv    | value to be assigned to the           |
   |             |          |            | bootloader variable                   |
   +-------------+----------+------------+---------------------------------------+
   | name        | string   | images     | name that identifies the sw-component |
   |             |          | files      | it can be any string and it is        |
   |             |          |            | compared with the entries in          |
   |             |          |            | sw-versions                           |
   +-------------+----------+------------+---------------------------------------+
   | version     | string   | images     | version for the sw-component          |
   |             |          | files      | it can be any string and it is        |
   |             |          |            | compared with the entries in          |
   |             |          |            | sw-versions                           |
   +-------------+----------+------------+---------------------------------------+
   | description | string   |            | user-friendly description of the      |
   |             |          |            | swupdate archive (any string)         |
   +-------------+----------+------------+---------------------------------------+
   | install-if\ | bool     | images     | flag                                  |
   | -different  |          | files      | if set, name and version are          |
   |             |          |            | compared with the entries in          |
   +-------------+----------+------------+---------------------------------------+
   | encrypted   | bool     | images     | flag                                  |
   |             |          | files      | if set, file is encrypted             |
   |             |          | scripts    | and must be decrypted before          |
   |             |          |            | installing.                           |
   +-------------+----------+------------+---------------------------------------+
   | data        | string   | images     | This is used to pass arbitrary data   |
   |             |          | files      | to a handler.                         |
   |             |          | scripts    |                                       |
   +-------------+----------+------------+---------------------------------------+
   | sha256      | string   | images     | sha256 hash of image, file or script. |
   |             |          | files      | Used for verification of signed       |
   |             |          | scripts    | images.                               |
   +-------------+----------+------------+---------------------------------------+
   | embedded-\  | string   |            | Lua code that is embedded in the      |
   | script      |          |            | sw-description file.                  |
   +-------------+----------+------------+---------------------------------------+
   | offset      | string   | images     | Optional destination offset           |
   +-------------+----------+------------+---------------------------------------+
   | hook        | string   | images     | The name of the function (Lua) to be  |
   |             |          | files      | called when the entry is parsed.      |
   +-------------+----------+------------+---------------------------------------+
   | mtdname     | string   | images     | name of the MTD to update. Used only  |
   |             |          |            | by the flash handler to identify the  |
   |             |          |            | the mtd to update, instead of         |
   |             |          |            | specifying the devicenode             |
   +-------------+----------+------------+---------------------------------------+
