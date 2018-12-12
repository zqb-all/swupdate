对称加密更新镜像
=====================================
SWUpdate允许在CBC模式下使用256位AES分组密码对更新镜像进行对称加密。


构建加密的SWU镜像
-------------------------------

首先，通过 `openssl` 创建密钥，这是openssl项目的一部分。
完整的文档可以在
`OpenSSL Website <https://www.openssl.org/docs/manmaster/man1/openssl.html>`_.
找到

::

        openssl enc -aes-256-cbc -k <PASSPHRASE> -P -md sha1

密钥和初始化向量是基于给定的 ``<PASSPHRASE>`` 生成的。
上述命令的输出如下:

::

        salt=CE7B0488EFBF0D1B
        key=B78CC67DD3DC13042A1B575184D4E16D6A09412C242CE253ACEE0F06B5AD68FC
        iv =65D793B87B6724BB27954C7664F15FF3

然后，使用这些信息加密图像:

::

        openssl enc -aes-256-cbc -in <INFILE> -out <OUTFILE> -K <KEY> -iv <IV> -S <SALT>

其中， ``<INFILE>`` 为未加密源镜像文件， ``<OUTFILE>`` 为加密后输出的镜像，
将在 ``sw-description`` 引用。
``<KEY>`` 是上述创建KEY命令得到的输出中，第二行的十六进制部分。
``<IV>`` 是第三行的十六进制部分，而 ``<SALT>`` 是第一行的十六进制部分。

然后，将密钥、初始化向量和盐的十六进制值放在由空格分隔的一行上，以创建一个
密钥文件，并在最终通过-k参数传递给SWUpdate.
例如，对于以上的示例数值

::

        B78CC67DD3DC13042A1B575184D4E16D6A09412C242CE253ACEE0F06B5AD68FC 65D793B87B6724BB27954C7664F15FF3 CE7B0488EFBF0D1B

注意，尽管不推荐，但为了向后兼容性，可以不带盐值使用OpenSSL。
要禁用盐值，请将 ``-nosalt`` 参数添加到上面的密钥生成命令中。
同时，在encryption命令中删除 ``-S <SALT>`` 参数，
并省略要提供给SWUpdate的密钥文件中的的第三个字段，即SALT。

UBI卷的加密
-------------------------

由于Linux内核api对UBI卷的限制，在实际写入任何内容之前，
需要声明要写入磁盘的数据大小。
不幸的是，加密映像的大小在完全解密之前是不知道的，
因此无法正确声明要写入磁盘的文件的大小。

出于这个原因，UBI镜像可以像这样声明特殊属性 "decrypted-size" :


::

	images: ( {
			filename = "rootfs.ubifs.enc";
			volume = "rootfs";
			encrypted = true;
			properties = {decrypted-size = "104857600";}
		}
	);


在组装cpio存档之前，应该计算解密图像的实际大小并将其写入sw-description。
在本例中，104857600是解密后rootfs的大小：加密后的大小会更大。


带加密镜像的sw-description例子
-------------------------------------------

下面的示例是一个(最小的) ``sw-description`` ，用于在Beaglebone上安装Yocto镜像。
注意 ``encryption = true;`` 的设置。

::

        software =
        {
        	version = "0.0.1";
        	images: ( {
        			filename = "core-image-full-cmdline-beaglebone.ext3.enc";
        			device = "/dev/mmcblk0p3";
        			encrypted = true;
        		}
        	);
        }



对加密映像运行SWUpdate
--------------------------------------

通过在SWUpdate的配置中设置 ``ENCRYPTED_IMAGES`` 选项，可以激活对称加密支持。
使用 `-K` 参数向SWUpdate提供上面生成的对称密钥文件。
