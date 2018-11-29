=======================================
Software Management on embedded systems
=======================================

Embedded Systems become more and more complex,
and their software reflects the augmented complexity.
New features and fixes let much more as desirable that
the software on an embedded system can be updated
in an absolutely reliable way.

On a Linux-based system, we can find in most cases
the following elements:

=======================================
嵌入式系统的软件管理
=======================================
嵌入式系统变得越来越复杂，
它们的软件也反映了这种复杂性的增加。
为了支持新的特性和修复，很有必要让嵌入式系统上的软件
能够以绝对可靠的方式更新。
在基于linux的系统上，我们可以在大多数情况下找到
以下元素:

- 引导装载程序
- 内核和设备树
- 根文件系统
- 其他在后续挂载的文件系统，other file systems, mounted at a later point
- 用户资料，以裸数据格式存在或者保存在文件系统中
- 特定用途的软件. 如，用于下载到相连接的微控制器的固件等



一般来说，在大多数情况下是需要更新
内核和根文件系统，保存用户数据-但实际情况各不相同。


仅在少数情况下，还需要更新引导加载程序，
事实上，更新引导加载程序总是很危险的，
因为更新中的失败会破坏设备。
在某些情况下，从损坏状态中恢复是可能的，
但这通常无法由最终用户完成，即设备需要返厂维修。

关于软件更新有很多不同的概念。
我将解释其中的一些概念，
然后解释为什么我实施了这个项目。

通过引导加载程序完成更新
================================

引导加载程序所做的工作远不止启动内核那么简单。
它们有自己的shell，且可以使用处理器的外围设备
进行管理，在大多数情况下是通过串行通讯。
它们通常是可执行脚本的，这使得
实现某种软件更新机制成为了可能。

然而，我发现这种方法有一些缺点，
这让我另行寻找基于运行在Linux上的应用程序的解决方案。

引导加载程序对外围设备的使用有局限性
-----------------------------------------------

并不是所有内核中支持的设备都可以在引导加载程序使用。
向内核添加设备支持是有意义的，因为这可以让外围设备对主应用程序可用，
但将驱动程序移植到引导加载程序中，就并不总是有意义的了。

引导加载程序的驱动程序不会被更新
-------------------------------------

引导加载程序的驱动程序大多是从Linux内核移植过来的，
但是由于经过调整的原因，它们以后不会被修复或与内核同步，
而bug修复则会定期在Linux内核中进行。

一些外围设备可能以不可靠的方式工作，
并且修复问题可能并不容易。引导加载程序中的驱动程序
或多或少是内核中相应驱动程序的复刻(fork)。

例如，用于NAND设备的UBI/UBIFS在内核中包含
了许多修复程序，这些修复程序并没有移植回引导加载程序。

USB协议栈也可以找到相同的情况。支持新外围设备或协议的工作,
在内核中进行得更好，而不是在引导加载程序中。

简化版的文件系统
--------------------

支持的文件系统的数量是有限的。
将文件系统支持移植到引导加载程序需要付出很大的努力。

网络支持有限
--------------------------

网络协议栈是有限的，通常通过一个更新只能通过
UDP但不能通过TCP完成。

与操作人员交互
-----------------------------

很难将接口暴露给操作员，
比如浏览器中的GUI或显示器上的GUI。

比起在引导加载程序中，复杂的逻辑可以在应用程序内部更容易实现。
扩展引导加载程序是复杂的，因为所有的服务和库都不可用。

引导加载程序更新的优点
-------------------------------
然而，这种方法也有一些优点：

-更新软件通常更简单。
-占用空间更小：即使是一个仅用于软件管理的独立应用程序
也需要自己的内核和根文件系统。即使它们的大小能够被裁剪，
将更新软件不需要的部分去掉，它们的大小也是不可忽略的。

Updating through a package manager
==================================

All Linux distributions are updating with a package manager.
Why is it not suitable for embedded ?

I cannot say it cannot be used, but there is an important drawback
using this approach. Embedded systems are well tested
with a specific software. Using a package manager
can put weirdness because the software itself
is not anymore *atomic*, but split into a long
list of packages. How can we be assured that an application
with library version x.y works, and also with different
versions of the same library? How can it be successfully tested?

For a manufacturer, it is generally better to say that
a new release of software (well tested by its test
engineers) is released, and the new software (or firmware)
is available for updating. Splitting in packages can
generate nightmare and high effort for the testers.

The ease of replacing single files can speed up the development,
but it is a software-versions nightmare at the customer site.
If a customer report a bug, how can it is possible that software
is "version 2.5" when a patch for some files were sent previously
to the customer ?

An atomic update is generally a must feature for an embedded system.


Strategies for an application doing software upgrade
====================================================

Instead of using the boot loader, an application can take
into charge to upgrade the system. The application can
use all services provided by the OS. The proposed solution
is a stand-alone software, that follow customer rules and
performs checks to determine if a software is installable,
and then install the software on the desired storage.

The application can detect if the provided new software
is suitable for the hardware, and it is can also check if
the software is released by a verified authority. The range
of features can grow from small system to a complex one,
including the possibility to have pre- and post- install
scripts, and so on.

Different strategies can be used, depending on the system's
resources. I am listing some of them.

Double copy with fall-back
--------------------------

If there is enough space on the storage to save
two copies of the whole software, it is possible to guarantee
that there is always a working copy even if the software update
is interrupted or a power off occurs.

Each copy must contain the kernel, the root file system, and each
further component that can be updated. It is required
a mechanism to identify which version is running.

SWUpdate should be inserted in the application software, and
the application software will trigger it when an update is required.
The duty of SWUpdate is to update the stand-by copy, leaving the
running copy of the software untouched.

A synergy with the boot loader is often necessary, because the boot loader must
decide which copy should be started. Again, it must be possible
to switch between the two copies.
After a reboot, the boot loader decides which copy should run.

.. image:: images/double_copy_layout.png

Check the chapter about boot loader to see which mechanisms can be
implemented to guarantee that the target is not broken after an update.

The most evident drawback is the amount of required space. The
available space for each copy is less than half the size
of the storage. However, an update is always safe even in case of power off.

This project supports this strategy. The application as part of this project
should be installed in the root file system and started
or triggered as required. There is no
need of an own kernel, because the two copies guarantees that
it is always possible to upgrade the not running copy.

SWUpdate will set bootloader's variable to signal the that a new image is
successfully installed.

Single copy - running as standalone image
-----------------------------------------

The software upgrade application consists of kernel (maybe reduced
dropping not required drivers) and a small root file system, with the
application and its libraries. The whole size is much less than a single copy of
the system software. Depending on set up, I get sizes from 2.5 until 8 MB
for the stand-alone root file system. If the size is very important on small
systems, it becomes negligible on systems with a lot of storage
or big NANDs.

The system can be put in "upgrade" mode, simply signaling to the
boot loader that the upgrading software must be started. The way
can differ, for example setting a boot loader environment or using
and external GPIO.

The boot loader starts "SWUpdate", booting the
SWUpdate kernel and the initrd image as root file system. Because it runs in
RAM, it is possible to upgrade the whole storage. Differently as in the
double-copy strategy, the systems must reboot to put itself in
update mode.

This concept consumes less space in storage as having two copies, but
it does not guarantee a fall-back without updating again the software.
However, it can be guaranteed that
the system goes automatically in upgrade mode when the productivity
software is not found or corrupted, as well as when the upgrade process
is interrupted for some reason.


.. image:: images/single_copy_layout.png

In fact, it is possible to consider
the upgrade procedure as a transaction, and only after the successful
upgrade the new software is set as "boot-able". With these considerations,
an upgrade with this strategy is safe: it is always guaranteed that the
system boots and it is ready to get a new software, if the old one
is corrupted or cannot run.
With U-Boot as boot loader, SWUpdate is able to manage U-Boot's environment
setting variables to indicate the start and the end of a transaction and
that the storage contains a valid software.
A similar feature for GRUB environment block modification as well as for
EFI Boot Guard has been introduced.

SWUpdate is mainly used in this configuration. The recipes for Yocto
generate an initrd image containing the SWUpdate application, that is
automatically started after mounting the root file system.

.. image:: images/swupdate_single.png

Something went wrong ?
======================

Many things can go wrong, and it must be guaranteed that the system
is able to run again and maybe able to reload a new software to fix
a damaged image. SWUpdate works together with the boot loader to identify the
possible causes of failures. Currently U-Boot, GRUB, and EFI Boot Guard
are supported.

We can at least group some of the common causes:

- damage / corrupted image during installing.
  SWUpdate is able to recognize it and the update process
  is interrupted. The old software is preserved and nothing
  is really copied into the target's storage.

- corrupted image in the storage (flash)

- remote update interrupted due to communication problem.

- power-failure

SWUpdate works as transaction process. The boot loader environment variable
"recovery_status" is set to signal the update's status to the boot loader. Of
course, further variables can be added to fine tuning and report error causes.
recovery_status can have the values "progress", "failed", or it can be unset.

When SWUpdate starts, it sets recovery_status to "progress". After an update is
finished with success, the variable is erased. If the update ends with an
error, recovery_status has the value "failed".

When an update is interrupted, independently from the cause, the boot loader
recognizes it because the recovery_status variable is in "progress" or "failed".
The boot loader can then start again SWUpdate to load again the software
(single-copy case) or run the old copy of the application
(double-copy case).

Power Failure
-------------

If a power off occurs, it must be guaranteed that the system is able
to work again - starting again SWUpdate or restoring an old copy of the software.

Generally, the behavior can be split according to the chosen scenario:

- single copy: SWUpdate is interrupted and the update transaction did not end
  with a success. The boot loader is able to start SWUpdate again, having the
  possibility to update the software again.

- double copy: SWUpdate did not switch between stand-by and current copy.
  The same version of software, that was not touched by the update, is
  started again.

To be completely safe, SWUpdate and the bootloader need to exchange some
information. The bootloader must detect if an update was interrupted due
to a power-off, and restart SWUpdate until an update is successful.
SWUpdate supports the U-Boot, GRUB, and EFI Boot Guard bootloaders.
U-Boot and EFI Boot Guard have a power-safe environment which SWUpdate is
able to read and change in order to communicate with them. In case of GRUB,
a fixed 1024-byte environment block file is used instead. SWUpdate sets
a variable as flag when it starts to update the system and resets the same
variable after completion. The bootloader can read this flag to check if an
update was running before a power-off.

.. image:: images/SoftwareUpdateU-Boot.png

What about upgrading SWUpdate itself ?
--------------------------------------

SWUpdate is thought to be used in the whole development process, replacing
customized process to update the software during the development. Before going
into production, SWUpdate is well tested for a project.

If SWUpdate itself should be updated, the update cannot be safe if there is only
one copy of SWUpdate in the storage. Safe update can be guaranteed only if
SWUpdate is duplicated.

There are some ways to circumvent this issue if SWUpdate is part of the
upgraded image:

- have two copies of SWUpdate
- take the risk, but have a rescue procedure using the boot loader.

What about upgrading the Boot loader ?
--------------------------------------

Updating the boot loader is in most cases a one-way process. On most SOCs,
there is no possibility to have multiple copies of the boot loader, and when
boot loader is broken, the board does not simply boot.

Some SOCs allow to have multiple copies of the
boot loader. But again, there is no general solution for this because it
is *very* hardware specific.

In my experience, most targets do not allow to update the boot loader. It
is very uncommon that the boot loader must be updated when the product
is ready for production.

It is different if the U-Boot environment must be updated, that is a
common practice. U-Boot provides a double copy of the whole environment,
and updating the environment from SWUpdate is power-off safe. Other boot loaders
can or cannot have this feature.
