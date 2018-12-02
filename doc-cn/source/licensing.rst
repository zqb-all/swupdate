=======
许可证
=======

SWUpdate是免费软件。它的版权属于Stefano Babic和其他
许多贡献代码的人(详情请参阅实际源代码和git提交信息)。
您可以根据自由软件基金会发布的GNU通用公共许可证第2版
的条款重新分发SWUpdate和/或修改它。
它的大部分还可以根据您的选择，在GNU通用公共许可证的
任何后续版本下发布——有关例外情况，请参阅个别文件。

为了更容易地表示许可证，源文件中的许可证头将被替换为
对由Linux基金会的SPDX项目[1]定义的唯一许可证标识符的一行引用。
例如，在源文件中，完整的“GPL v2.0或更高版本”标题文本将被一行替换:

::

	SPDX-License-Identifier:	GPL-2.0+

理想情况下，源码树中所有文件的许可证条款都应该由这样的许可证
标识符定义；在任何情况下，文件都不能包含一个以上的许可证标识符列表。

如果“SPDX-License-Identifier:”行引用了多个不同的许可证标识符，
则这意味着可以在这些许可证中的任意一个的条款下使用相应的文件,
例如，若带有如下标志

::

	SPDX-License-Identifier:	GPL-2.0+	BSD-3-Clause

则您可以在 GPL-2.0+和 BSD-3-Clause 许可证之间进行选择。

我们使用 SPDX_ 唯一许可标识符(SPDX-identifiers_)

.. _SPDX: http://spdx.org/
.. _SPDX-Identifiers: http://spdx.org/licenses/

.. table:: Licenses

   +-------------------------------------------------+------------------+--------------+
   | Full name                                       |  SPDX Identifier | OSI Approved |
   +=================================================+==================+==============+
   | GNU General Public License v2.0_ only           | GPL-2.0-only     |    Y         |
   +-------------------------------------------------+------------------+--------------+
   | GNU General Public License v2.0_ or later       | GPL-2.0-or-later |    Y         |
   +-------------------------------------------------+------------------+--------------+
   | GNU Lesser General Public License v2.1_ or later| LGPL-2.1-or-later|    Y         |
   +-------------------------------------------------+------------------+--------------+
   | BSD 2-Clause_ License                           | BSD-2-Clause     |    Y         |
   +-------------------------------------------------+------------------+--------------+
   | BSD 3-clause_ "New" or "Revised" License        | BSD-3-Clause     |    Y         |
   +-------------------------------------------------+------------------+--------------+
   | MIT_ License                                    | MIT              |    Y         |
   +-------------------------------------------------+------------------+--------------+

.. _v2.0: http://www.gnu.org/licenses/gpl-2.0.txt
.. _v2.1: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt
.. _2-Clause: http://spdx.org/licenses/BSD-2-Clause
.. _3-Clause: http://spdx.org/licenses/BSD-3-Clause#licenseText
.. _MIT: https://spdx.org/licenses/MIT.html
