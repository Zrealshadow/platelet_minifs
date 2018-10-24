#ifndef FUNC_CORE_H
#define FUNC_CORE_H
#include"version.h"
#include"fs_core.h"
#include<string>
#include<QString>
#include<func_macro.h>
#include<fs_macro.h>
namespace func {
//*--------------------功能模块----------------------*
    void create(QString name);
    //创建文件系统，并命名为name

    void mount(QString PATH,QString name);
    //挂载name文件系统
    void mount_display();
    //展示已挂载文件系统对应的名字
    void unmount(QString name);
    //卸载name文件系统

    void fm(QString path);
    //格式化当前文件系统

    void close();
    //关闭当前文件系统

    void mk_create_file(QString name);
    void mk_create_dir(QString name);
    //mk 新建操作 ,创建成功，打印提示消息，创建失败，打印对应error

    void dr_prt_file(QString PATH);
    //dr 打印操作，打印出当前目录下的所有文件

    void cp_copy_file(QString in_PATH,QString out_PATH);
    void cp_copy_dir(QString in_PATH,QString out_PATH);
    //cp 复制操作，复制成功，打印提示消息，复制失败，打印对应error`


    void dl_delete_file(QString PATH);
    void dl_delete_dir(QString PATH);
    //dl 删除操作，删除成功，打印提示消息，删除失败，打印对应error

    void tp_display(QString PATH);
    //tp 展示操作 打印文件内容

    void tr_display();
    //tree 树型打印出当前目录下所有子目录和文件

    void at_display(QString NAME);
    //att 打印文件属性

    void mp_display(QString PATH);
    //map 展示文件系统下所利用的block

    void mv_move_file(QString in_PATH,QString out_PATH);
    void mv_move_dir(QString in_PATH,QString out_PATH);
    //mv 移动操作，移动成功，打印提示消息，移动失败，打印对应error

    void cd_change_dir(QString PATH);
    //cd 切换操作，切换目录成功，打印当前目录路径，切换失败，打印对应error

    void ff(QString NAME);
    //find 查询操作，在current路径下查找名为name的文件或文件夹，成功，打印文件路径，失败，打印对应error

    void help();
    void help(int hash);


    void in_rewrite(QString PATH,QString message);
    //in 写入操作  重写文件内容

    void name_rename(QString former_name,QString current_name);
    //name 重命名操作


//*-------------------功能辅助函数----------------------*

    void tree_dfs(int deep,QList<int>is_last,QString current_path);
    //tree 功能辅助函数 递归函数

    void ff_dfs(int deep,QString NAME,QString path);
    //find 功能辅助函数 递归函数

    void prt_file(QString name);
    //文件 GREEN 打印

    void prt_dir(QString name);
    //文件夹 BOLDBLUE 打印

    QString where_path();
    //返回当前文件夹路径 便于显示

    void cp_sys_copy_dir(QString in_PATH,QString out_PATH);
    void cp_sys_copy_file(QString in_PATH,QString out_PATH);
    void icp_copy_file(QString in_PATH,QString out_PATH);
    void ocp_copy_file(QString in_PATH,QString out_PATH);
    //cp 内外部复制操作，复制成功，打印提示消息，复制失败，打印对应error`

}
#endif // FUNC_CORE_H
