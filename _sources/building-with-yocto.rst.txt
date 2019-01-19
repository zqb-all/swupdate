==================================
meta-swupdate: 使用Yocto进行编译
==================================

概览
========

Yocto-Project_ 是Linux基金会旗下的一个社区项目，
它提供了为嵌入式系统创建自定义的基于Linux的软件的工具和模板。

.. _Yocto-Project: http://www.yoctoproject.org
.. _meta-SWUpdate:  https://github.com/sbabic/meta-swupdate.git

可以使用 *layers* 添加附加功能。meta-swupdate_ 是交叉编译SWUpdate应用程序
并生成包含发布产品的复合SWU镜像的层。正如Yocto关于层的文档中所描述的，
你应该在 *bblayers.conf* 中包含它，以使用它。

swupdate 类
==================

meta-sWUpdate 包含用于SWUpdate的类。它有助于基于Yocto中的构建的镜像生成SWU镜像。
它要求所有组件(即将要构成SWU映像的构件)都包含在Yocto的deploy目录中。
这个类应该由生成SWU的菜谱继承。这个类定义了一些新的变量，
它们的名称中都有前缀 *SWUPDATE_* 。

- **SWUPDATE_IMAGES**:这是要打包在一起的工件的列表。
  该列表包含自动添加的没有任何机器或文件类型扩展名的图像的名称。
  例如:

::

        SWUPDATE_IMAGES = "core-image-full-cmdline uImage"

- **SWUPDATE_IMAGES_FSTYPES**:工件的扩展名。根据IMAGE_FSTYPES变量，每个工件可以有
  多个扩展名。例如，可以将镜像生成为目标的tarball和UBIFS。
  为每个工件设置变量可以告诉类必须将哪个文件打包到SWU映像中。

::

        SWUPDATE_IMAGES_FSTYPES[core-image-full-cmdline] = ".ubifs"

- **SWUPDATE_IMAGES_NOAPPEND_MACHINE**:用于指示从工件文件名中删除机器名称的标志。
  *deploy* 中的大多数镜像在文件名中都有Yocto机器的名称。
  该类会自动将机器的名称添加到文件中，但是有些工件可以在没有它的情况下进行部署。

::

        SWUPDATE_IMAGES_NOAPPEND_MACHINE[my-image] = "1"

- **SWUPDATE_SIGNING** : 如果设置，SWU将被签名。有3个允许的值:RSA, CMS, CUSTOM。
  此值确定使用的签名机制。
- **SWUPDATE_SIGN_TOOL** : 使用SWUPDATE_SIGN_TOOL来签名，而不是使用openssl。
  一个典型的用例是与硬件密钥一起使用。此变量在swupdate_sign设置为CUSTOM时生效。
- **SWUPDATE_PRIVATE_KEY** : 这个私钥用于使用RSA机制对镜像进行签名。
  此变量在swupdate_sign设置为RSA时生效。
- **SWUPDATE_PASSWORD_FILE** : 一个可选的包含私钥密码的文件。
  此变量在swupdate_sign设置为RSA时生效。
- **SWUPDATE_CMS_KEY** : 使用CMS机制签名过程中使用的私钥。
  此变量在swupdate_sign设置为CMS时生效。
- **SWUPDATE_CMS_CERT** : 使用CMS机制签名过程中使用的证书文件。
  此变量在swupdate_sign设置为CMS时生效。

sw-description中的自动sha256
----------------------------------

swupdate类负责计算并在sw-description文件中插入sha256散列。
如果镜像有签名，则 **必须** 设置属性 *sha256* 。每个工件必须具有以下属性:

::

        sha256 = "@artifact-file-name"

例如，将sha256添加到标准的Yocto core-image-full-cmdline中:

::

        sha256 = "@core-image-full-cmdline-machine.ubifs";

文件的名称必须与deploy目录中的名称相同。

sw-description 中的 BitBake 变量展开
--------------------------------------------

若要将位Bitbake变量的值插入到更新文件中，请在变量名的前后加上“@@”。
例如，要自动设置版本标签:

::

        version = "@@DISTRO_VERSION@@";

使用该类的菜谱的模板
-----------------------------------

::

        DESCRIPTION = "Example recipe generating SWU image"
        SECTION = ""

        LICENSE = ""

        # Add all local files to be added to the SWU
        # sw-description must always be in the list.
        # You can extend with scripts or wahtever you need
        SRC_URI = " \
            file://sw-description \
            "

        # images to build before building swupdate image
        IMAGE_DEPENDS = "core-image-full-cmdline virtual/kernel"

        # images and files that will be included in the .swu image
        SWUPDATE_IMAGES = "core-image-full-cmdline uImage"

        # a deployable image can have multiple format, choose one
        SWUPDATE_IMAGES_FSTYPES[core-image-full-cmdline] = ".ubifs"
        SWUPDATE_IMAGES_FSTYPES[uImage] = ".bin"

        inherit swupdate
