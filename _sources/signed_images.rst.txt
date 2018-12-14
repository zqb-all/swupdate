从可信的来源更新镜像
==================================

现在越来越重要的是，设备不仅要能安全地进行更新操作，
而且要能够验证发送的图像是否来自一个已知的源，
并且没有嵌入恶意软件。

为了实现这个目标，SWUpdate必须验证传入的镜像。
有几种方法可以做到这一点。
这里有一些问题，完整的复合镜像需要签名吗?还是只是它的某些部分需要?

不同做法的优缺点将在下一章中描述。

对复合镜像进行签名
--------------------------

一个直接了当的做法是对整个复合镜像进行签名。但是。这样做有一些严重
的缺点。这会导致无法在加载完整个复合镜像之前对镜像进行验证。
这意味着，校验需要在安装了镜像之后才进行，而不是在实际写入设备
之前就能进行。
这会导致，如果校验失败，需要对已经安装好的镜像做一些取消安装的操作，
这种取消安装的操作，在碰到掉电时，可能会导致一些不希望保留的数据被保留在设备上。

对子镜像进行签名
----------------------

如果每个子图像都签名了，验证就可以在操作相应的硬件之前完成。
只有签名正确的镜像会被实际安装。
不过这样存在一个问题，子镜像没有跟sw-descrription文件中的发布描述绑定到一起。
即使sw-description也做了签名，即使对sw-description进行了签名，攻击者也可以
将签名子镜像们混合在一起，生成可以安装的新的复合镜像，因为所有子镜像都可通过验证。

对sw-description进行签名并与哈希验证相结合
-------------------------------------------------------

为了避免所描述的缺点，SWUpdate将签名的sw-description与每个子镜像的哈希验证结合起来。
这意味着只有经过验证的源代码生成的sw-description才能被安装程序接受。
而sw-description包含每个子镜像的哈希值，可验证每个交付的子镜像确实属于本次发布。

算法的选择
-------------------

可以通过menuconfig选择签名和验证sw-descrription文件的算法。
目前，实现了以下机制:


- RSA 公钥/私钥。 私钥属于编译系统，而公钥需要被安装到设备上。
- 使用证书的CMS

密钥或证书使用"-k"参数传递给SWUpdate。

生成密钥/证书的工具
------------------------------------

`openssl` 工具用于生成密钥。这是OpenSSL项目的一部分。完整的文档可以
在 `openSSL 网站 <https://www.openssl.org/docs/manmaster/man1/openssl.html>`_ 上找到


使用 RSA PKCS#1.5
-----------------------

生成私钥和公钥
.................................

首先，需要生成私钥

::

        openssl genrsa -aes256 -out priv.pem

这里需要一个密码。可以从文件中去获取这个密码 - 当然，
这个密码文件必须保护好，防止被入侵。

::

        openssl genrsa -aes256 -passout file:passout -out priv.pem

使用如下命令，从私钥导出公钥:

::

        openssl rsa -in priv.pem -out public.pem -outform PEM -pubout

"public.pem" 包含了适用于swupdate的格式的密钥。
该文件可以通过-k参数在命令行传递给swupdate。


如何使用RSA进行签名
....................

对镜像进行签名非常简单:

::

        openssl dgst -sha256 -sign priv.pem sw-description > sw-description.sig


与证书和CMS一起使用
-------------------------------


生成自签名证书
...................................

::

        openssl req -x509 -newkey rsa:4096 -nodes -keyout mycert.key.pem \
            -out mycert.cert.pem -subj "/O=SWUpdate /CN=target"

有关参数的更多信息，请参阅文档。
"mycert.key.pem" 包含了私钥，用于签名。
它 *不能* 被部署到目标设备上。

目标设备上必须安装有 "mycert.cert.pem" - 这将被SWUpdate用于完成校验。


使用PKI颁发的证书
.............................

也可以使用PKI签发的代码签名证书。
不过，SWUpdate是使用OpenSSL库来处理CMS签名的，该库要求在签名证书上设置以下属性:

::

        keyUsage=digitalSignature
        extendedKeyUsage=emailProtection

如果不能满足此要求，也可以完全禁用签名证书密钥检查。
这是由 `CONFIG_CMS_IGNORE_CERTIFICATE_PURPOSE` 配置选项控制的。


如何用CMS签名
.....................

对镜像进行签名，跟前一种情况一样很简单:

::

        openssl cms -sign -in  sw-description -out sw-description.sig -signer mycert.cert.pem \
                -inkey mycert.key.pem -outform DER -nosmimecap -binary


构建签名的SWU镜像
---------------------------

有两个文件，sw-description和它的签名sw-description.sig。
签名文件必须紧跟在描述文件后面。

sw-description中的每个图像必须具有 "sha256" 属性，
即镜像的sha256校验和。如果有一个镜像不具有sha256属性，
则整个复合镜像的的校验结果会是未通过，SWUpdate在开始安装之前会停止并报错。

创建签名镜像的简单脚本可以是:

::

        #!/bin/bash

        MODE="RSA"
        PRODUCT_NAME="myproduct"
        CONTAINER_VER="1.0"
        IMAGES="rootfs kernel"
        FILES="sw-description sw-description.sig $IMAGES"

        #if you use RSA
        if [ x"$MODE" == "xRSA" ]; then
            openssl dgst -sha256 -sign priv.pem sw-description > sw-description.sig
        else
            openssl cms -sign -in  sw-description -out sw-description.sig -signer mycert.cert.pem \
                -inkey mycert.key.pem -outform DER -nosmimecap -binary
        fi
        for i in $FILES;do
                echo $i;done | cpio -ov -H crc >  ${PRODUCT_NAME}_${CONTAINER_VER}.swu



签名镜像的sw-description示例
--------------------------------------------

本例应用于Beaglebone，安装Yocto images:


::

        software =
        {
                version = "0.1.0";

                hardware-compatibility: [ "revC"];

                images: (
                        {
                            filename = "core-image-full-cmdline-beaglebone.ext3";
                            device = "/dev/mmcblk0p2";
                            type = "raw";
                            sha256 = "43cdedde429d1ee379a7d91e3e7c4b0b9ff952543a91a55bb2221e5c72cb342b";
                        }
                );
                scripts: (
                        {
                            filename = "test.lua";
                            type = "lua";
                            sha256 = "f53e0b271af4c2896f56a6adffa79a1ffa3e373c9ac96e00c4cfc577b9bea5f1";
                         }
                );
        }


对签名镜像运行SWUpdate
-----------------------------------

验证是通过在SWUpdate的配置中设置CONFIG_SIGNED_IMAGES激活的。
一旦激活，SWUpdate将始终检查复合图像。
出于安全原因，不可能在运行时禁用检查。
-k参数(公钥文件)是必须的，如果公钥没有传递，程序将终止运行。
