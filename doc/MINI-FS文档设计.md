# MINI_FS 微型文件系统设计文档

> Author : 欧阳峻彦 曾令泽 侯佳成 辛高枫 张洛汐
>
> Version：1.0v

















[TOC]



























## 1.需求分析与初步设计

#### 功能要求

|  命令名   |         命令参数         |             命令功能             |
| :----: | :------------------: | :--------------------------: |
| create |     create SName     |   建立名为SName大小为1G的文件作为储存空间    |
| mount  |     mount SName      | 在mini-FS中安装空间SName，为后续操作进行准备 |
|  fmt   |          无           |        格式化当前 SName 空间        |
|   mk   |    mk -f(-d)sname    |     新建一个名为sname的文件(文件夹)      |
|   dr   |          无           |        显示空间中的文件目录：dir        |
|   cp   | cp -f(-d) sname PATH |    复制文件(文件夹）sname到PATH路径下    |
|   dl   |   dl -f(-d) sname    |      删除空间中的文件(文件夹)sname      |
|   tp   |          无           |       显示空间中的文本文件：type        |
|  more  |    more filename     |      分页显示空间中的文件filename      |
|  att   |     att filename     |       显示空间文件filename属性       |
|   mv   | mv -f(-d) name path  |    移动文件（文件夹）name到path路径下     |
|  find  |   find -f(-d) name   |     在当前目录下索引文件(文件夹）name      |
|  help  |          无           |            打印帮助信息            |
| close  |          无           |         退出空间，返回操作系统          |

#### 其他需求

- - 能够充分利用存储空间，存储效率比较高
  - 删除文件的空间能够循环利用
  - 算法实现不能太复杂，效率尽可能高，执行时间不能过长







## 2.数据结构初步设计

#### mini-FS整体架构

整个1G的空间被分为了5个部分，每个部分记录相应的信息，5个部分分别为：

- mini-FS的格式信息
- inode的位图信息
- block 的位图信息
- 储存inode的数组
- 储存block的数组

![3CD82E79-EC5F-4CA3-91BE-F627567D240D](assets/3CD82E79-EC5F-4CA3-91BE-F627567D240D.png)下面分别介绍每个部分储存的内容

###### mini-FS格式信息

这个处于文件开头的mini-SF的配置信息块是需要最先读入内存的，其中包括后面4个模块开头的位置相对于文件开头的偏置bias以及每个模块的大小，inode的大小，block的大小，inode 数，block数 等基本信息。
|      格式信息      |
| :------------: |
| inode位图模块位置及大小 |
| block位图模块位置及大小 |
| inode数组模块位置及大小 |
| block数组模块位置及大小 |
|   inode块的大小    |

###### inode位图和Block位图

使用bitmap数据结构储存inode数组模块和block数组模块中空间的利用情况。如果某个某个block或inode被利用，则位图中对应的bit置1，否则置0.

###### inode数组

inode数组里面储存的是顺序排列的inode数据类型，文件系统中每个文件有唯一与之对应的inode。定义每个inode可以指向一个或多个block，又或是一个或多个inode（之后会进行说明）。其中存储的信息分为两个部分，一个是文件的信息与inode自身的信息，例如文件类型，版本号，文件wr权限，文件大小，创建时间等等，第二个部分存block数组索引，或inode数组中的索引，我们利用一个多级索引的结构，一级索引，可以直接索引对应的内容block；二级索引，先找到相应的inode，再通过找到的inode中的一级索引，寻找对应的block。三级索引同理。



![DAF41E9C2354C4AAFF95FE62F3438B20](assets/DAF41E9C2354C4AAFF95FE62F3438B20.jpg)

###### block数组

block数组模块中是顺序储存的内容块，有两类block，一类是data block，一类是list block。 

data block 为二进制数据块，一个文件的内容以二进制的形式被存储在若干个这样的data block中。

而对于list block，我们用一个文件内容为文件夹子文件inode索引的文件来来模拟文件夹的功能，list block中存储的就是该文件夹的父文件夹，子文件夹以及子文件的inode索引。通过这些索引我们很容易实现文件夹的切换，文件的索引。

![E51FA151-ADA4-4668-BF8D-6098A346F7EA](assets/E51FA151-ADA4-4668-BF8D-6098A346F7EA.png)



#### mini-FS数据结构及接口设置 

核心域名

```C++
namespace fs{
    //挂载minifs
    //参数为外部文件系统路径，返回FS对象的引用（const）
    //FS表示一个minifs文件系统
    const FS& mount(char* outer_path_);
    
    
    //打开minifs中的文件
    //参数：path_为内部路径，mode_为读写模式（有'w','r','wb','rb'等）
    //返回FSFile对象
    FSFile open(char* path_, char* mode_, FS& minifs_);
    
    
    //关闭fs::open打开的文件
    //参数：fs::open打开的FSFile
    void close(FSFile fs_file_)；
    
    
    class FSFile
    {
        public:
        	//用于文本文件读取,offset为偏置
        	void read(char* str_,int offset);
        
        	//用于二进制文件读取
        	//ptr_obj_为数组头（读取大小为size_*nmemb），size_为每个数据的大小，nmemb_为个数
        	void read(void* ptr_obj_,int size_ ,int nmemb_,int offset);
        
        	//用于文本文件写入
        	void write(char* str_,int offset);
        
        	//用于二进制文件写入
        	void write(void* ptr_obj_, int size_,int nmemb_,int offset);
        
        
        
        	//移动操作，把文件移动到new_path
        	void mv_to(char* new_path);
        
        	//设置只读
        	void set_read_only(bool read_only=true);
        
        	//获取属性
        	FSFileAttr& get_attr();
        
        	//删除文件
        	friend void rm(FSFile& FSFile)
    }
    
    typedef struct FSFileAttr{
        char file_type; //文件类型
        int auth_mod;//文件权限数
        int size; //文件大小
        long long create_time;//创建时间
        long long latest_access_time;//最近访问时间
        long long latest_content_change_time;//最近内容修改时间
        long long latest_attr_change_time;//最近属性变更时间
    }FSFileAttr;
    
}
```





## 3.命令的实现描述

**显示空间中的目录文件（dr操作）**

*需要参数：无*

*操作结果：打印出以现有目录为根目录下的所有的目录文件*

遍历当前目录文件的目录块，取出其中文件名信息，存为数组，最后打印。



**拷贝文件（cp操作）**

*需要参数：源文件的路径PATH1  拷贝文件的路径PATH2*

*操作结果：返回一个显示拷贝是否成功值，并将其打印*



（1）拷贝文件到空间：

申请新inode，根据PATH2在新位置上级目录块中添加新文件的文件名与inode索引。再递归地拷贝所有源文件信息、数据，通过miniFS的存储方式存储到空间中（内存只作中转站，每次被拷贝的单位为块）。修改bitmap。

（2）拷贝空间文件到外部：

读取对应文件，再把信息写入外部文件。





**删除文件（dl操作）**

*需要参数：源文件的路径 PATH*

*操作结果：返回一个显示删除是否成功的值，并将其打印*



根据PATH到对应文件，并获取相关的所有inode和block索引，根据索引在相应bitmap中的对应位置置0。



**显示空间文本文件（tp操作）**

*需要参数：源文件的绝路径PATH*

*操作结果：返回文件的内容，并将其打印*



根据源文件的路径找到该文件inode，根据inode中的索引信息，按顺序找到各data_block读取文本文件，将其打印。





**移动文件（mv操作）**

*需要参数：源文件的路径PATH1 文件移动目的地的路径PATH2*

*操作结果：返回一个显示移动是否成功的值，并将其打印*

修改文件目录块A对应的上级目录块P1，去除P1中A的信息，并根据PATH找到新上级目录块P2，在其中加上A的信息（即文件名和inode索引）



**新建文件或文件夹（mk操作）**

*需要参数：需要新建文件的路径PATH*   新建文件还是文件夹 （-d/-f)

*操作结果：在PATH下创建一个新的文件*



根据PATH，先申请一个inode，在上级目录的目录块中增加一条记录（inode与文件名），如果是新建文件，暂时不给该文件的inode分配任何数据块，如果是目录文件，则要在目录块中增加 . (当前目录)与 ..（上级目录）并做好配置。



**查询文件（find操作）**

*需要参数：索引文件名*

*操作结果：返回一个表示是否查询成功的值，如果查询成功打印索引文件的绝对路径*

在当前目录下（以后可能会指定目录），进行递归遍历，找到文件名称与所给字符串匹配的文件（找到该文件名的记录所在的目录块），并返回该文件绝对路径。





## 4. 其他



1. #### 常驻内存的信息：

   1. 基本配置信息
   2. inode bitmap，用位压缩的方式存储inode的利用情况
   3. block bitmap，用位压缩的方式存储block的利用情况
   4. 该文件系统根目录文件（默认为第一个inode和第一个block）
   5. 用于寻找bitmap中空闲位的数据结构（初步确定使用最小堆）
   6. inode哈希表，block哈希表。用于缓存数据。对频繁访问同一块的情况提升较大。
   7. （暂时想到这么多）

2. #### 如何根据需要读取要处理的部分:

   1. 根据inode的指示逐个地读取block信息。先根据一级索引，后是二级，三级。
   2. 不管是inode还是block，读取前都要计算哈希值，如果哈希表中存在副本，就不用到外存中读取。

3. #### 内存如何和外存同步：

   1. 首先是bitmap，要对其变更时，内存中的bitmap首先变更，因为要快速告诉别的进程某块inode或block正在被占用。写入时，bitmap是最后写入的，防止在内容被写入数据块中途中断操作导致数据不完整，因为这样写意味着只有写操作进行完的块才可以在bitmap中被置1。
   2. 哈希表的作用主要体现在读取部分。写部分，采用写直达方式，内存和外存同时变更（暂时这样设计，但是这样对效率没有提升）。

4. #### 软件的基本结构：

   1. 暂时未完成设计

















