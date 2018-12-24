=====================
Mongoose 守护进程模式
=====================

介绍
------------

Mongoose是SWUpdate的守护模式，它提供了web服务器、web接口和web应用程序。

Mongoose支持两个不同的web接口版本，但第一个版本已被弃用，不应该使用它。
第二个版本使用WebSocket在web服务器和web应用程序之间进行异步通信，
允许镜像更新过程的可视化，通过更新后命令重新启动系统，
并在重新启动或连接丢失后自动重新加载web页面。

``web-app`` 中的web应用程序使用 `Node.js`_ 包管理和 `gulp`_ 作为构建工具。
它依赖于 `Bootstrap 4`_, `Font Awesome 5`_ 和 `Dropzone.js`_.

.. _Node.js: https://nodejs.org/en/
.. _gulp: https://gulpjs.com/
.. _Bootstrap 4: https://getbootstrap.com/
.. _Font Awesome 5: https://fontawesome.com/
.. _Dropzone.js: http://www.dropzonejs.com/


启动
-------

在配置和编译了启用了mongoose web服务器和web接口版本2支持的SWUpdate之后，

.. code:: bash

  ./swupdate --help

列出必须提供给mongoose的强制参数和可选参数。
作为一个例子,

.. code:: bash

  ./swupdate -l 5 -w '-r ./examples/www/v2 -p 8080' -p 'reboot'

使用日志级别 ``TRACE`` 运行SWUpdate的mongoose守护模式，
在 http://localhost:8080 上运行web服务器。


例子
-------

开箱即用的web应用程序例子在 ``examples/www/v2`` 目录下，
使用了一个来自 `pixabay`_ 的公共域 `background.jpg` 图像,
该图像时在知识共享协议CC0许可下发布的。
使用的 `favicon.png` 和 `logo.png` 图片是从SWUpdate logo生成的，
因此受GNU通用公共许可证第2版的约束。你必须遵守本许可证或将图片
替换为你自己的文件。

.. _pixabay: https://pixabay.com/de/leiterbahn-platine-technologie-3157431/


定制
---------

你可以在 ``web-app`` 目录中定制web应用程序。除了 替换 `favicon.png`
, `logo.png` 和 `background.jpg` 之外，你可以在 ``scss/bootstrap.scss``
样式表中自定义Bootstrap颜色和设置。
更改样式表之后，需要重新从源码构建web应用程序。


开发
-------

开发需要Node.js版本6或更高版本，以及一个支持mongoose web服务器
和web应用程序接口版本2的预构建SWUpdate项目。

#. 进入web应用程序目录::

    cd ./web-app

#. 安装依赖::

    npm install

#. 构建web应用程序::

    npm run build

#. 启动web应用程序::

    ../swupdate -w '-r ./dist -p 8080' -p 'echo reboot'

#. 测试web应用程序:

    http://localhost:8080/

#. 打包web应用程序(可选的)::

    npm run package -- --output swupdate-www.tar.gz


贡献
----------

请在做任何提交之前先运行lint

.. code:: bash

    npm run lint
