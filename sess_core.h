#ifndef SESS_CORE_H
#define SESS_CORE_H
#include <QString>
#include "version.h"
#include "func_core.h"
#include <stdio.h>
#include <iostream>
#include <map>

namespace sess{
//*--------------------会话模块----------------------*
    void parse_cmd(QString cmd,std::map<std::string,int>map_cmd);
    //解析命令 进行特判

    void user_interface();
    //用户界面

    void parse_create(QString command);
    //解析create命令，并判断输入格式是否规范

    void parse_mount(QString command);
    //解析mount命令，并判断输入格式是否规范

    void parse_unmount(QString command);
    //解析unmount命令，并判断输入格式是否规范

    void parse_format(QString command);
    //判断输入格式是否规范

    void parse_close(QString command);
    //判断输入格式是否规范

    void parse_make(QString command);
    //解析mk命令，并判断输入格式是否规范

    void parse_dr(QString command);
    //解析dr命令，并判断输入格式是否规范

    void parse_cp(QString command);
    //解析cp命令，并判断输入格式是否规范

    void parse_delete(QString command);
    //解析dl命令，并判断输入格式是否规范

    void parse_tp(QString command);
    //解析tp命令，并判断输入格式是否规范

    void parse_tree(QString command);
    //解析tree命令，并判断输入格式是否规范

    void parse_att(QString command);
    //解析att命令，并判断输入格式是否规范

    void parse_move(QString command);
    //解析mv命令，并判断输入格式是否规范

    void parse_help(QString command);
    //解析help命令，并判断输入格式是否规范

    void parse_in(QString command);
    //解析in命令，并判断输入格式是否规范

    void parse_find(QString command);
    //解析find命令，并判断输入格式是否规范

    void parse_name(QString command);
    //解析name命令，并判断输入格式是否规范

    void parse_map(QString command);
    //解析map命令，并判断输入格式是否规范

    void parse_cd(QString command);
    //解析cd命令，并判断输入格式是否规范

//*-------------------辅助函数----------------------*
    void prt_warning(QString command);
    //打印警告

}
#endif//FUNC_FUNC_H
