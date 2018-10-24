#include "sess_core.h"


namespace sess {
//*------------------会话层内部参数------------------------*
std::map<std::string,int> map_cmd;
bool fmt_flag;

//*-----------------会话实现辅助函数-------------------------*

//创造命令map
std::map<std::string,int> create_map(){
    std::map<std::string,int>map_cmd;
    map_cmd.insert(std::pair<std::string,int>("create",1));
    map_cmd.insert(std::pair<std::string,int>("mount",2));
    map_cmd.insert(std::pair<std::string,int>("fmt",3));
    map_cmd.insert(std::pair<std::string,int>("dr",4));
    map_cmd.insert(std::pair<std::string,int>("cp",5));
    map_cmd.insert(std::pair<std::string,int>("dl",6));
    map_cmd.insert(std::pair<std::string,int>("tp",7));
    map_cmd.insert(std::pair<std::string,int>("att",8));
    map_cmd.insert(std::pair<std::string,int>("mv",9));
    map_cmd.insert(std::pair<std::string,int>("find",10));
    map_cmd.insert(std::pair<std::string,int>("help",11));
    map_cmd.insert(std::pair<std::string,int>("map",12));
    map_cmd.insert(std::pair<std::string,int>("tree",13));
    map_cmd.insert(std::pair<std::string,int>("close",14));
    map_cmd.insert(std::pair<std::string,int>("in",15));
    map_cmd.insert(std::pair<std::string,int>("mk",16));
    map_cmd.insert(std::pair<std::string,int>("cd",17));
    map_cmd.insert(std::pair<std::string,int>("name",18));
    map_cmd.insert(std::pair<std::string,int>("unmount",19));
    return map_cmd;
}

//特判命令
int map_find(std::string command){
    std::map<std::string,int>::iterator it=map_cmd.find(command);
    if(it!=map_cmd.end())
        return it->second;
    else
        return -1;
}

//*-----------------会话模块函数---------------------------*

//解析create命令，并判断输入格式是否规范
void parse_create(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==2)
        func::create(cmd_list.value(1));
    else{
        prt_warning(cmd_list.value(0));
    }
}

//解析mount命令，并判断输入格式是否规范
void parse_mount(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==3)
        func::mount(cmd_list.value(1),cmd_list.value(2));
    else{
        prt_warning(cmd_list.value(0));
    }
}

//解析unmount命令，并判断输入格式是否规范
void parse_unmount(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==1)
        func::mount_display();
    else if(cmd_list.size()==2)
        func::unmount(cmd_list.value(1));
    else{
        prt_warning(cmd_list.value(0));
    }
}

//解析fmt命令，并判断输入格式是否规范
void parse_format(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==2)
        func::fm(cmd_list.value(1));
    else{
        prt_warning(cmd_list.value(0));
}
}

//解析close命令，并判断输入格式是否规范
void parse_close(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==1)
        func::close();
    else{
        prt_warning(cmd_list.value(0));
}
}

//解析mk命令，并判断输入格式是否规范
void parse_make(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    QString type=cmd_list.value(1);
    if(type!="-d"&&type!="-f")
        prt_warning((cmd_list.value(0)));
    else if(type=="-d")
        for(int i=2;i<cmd_list.size();i++)
            func::mk_create_dir(cmd_list.value(i));
        else
        for(int i=2;i<cmd_list.size();i++)
            func::mk_create_file(cmd_list.value(i));
}

//解析dr命令，并判断输入格式是否规范
void parse_dr(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    switch (cmd_list.size()) {
    case 1:func::dr_prt_file(".");break;
    case 2:func::dr_prt_file(cmd_list.value(1));break;
    default: prt_warning(cmd_list.value(0));
    }
}

//解析cp命令，并判断输入格式是否规范
void parse_cp(QString command){
    QString flag=command.section(' ',1,1);
    QString in_PATH=command.section(' ',2,2);
    QString out_PATH=command.section(' ',3,3);
    if(flag=="-f")
        func::cp_copy_file(in_PATH,out_PATH);
    else if(flag=="-d")
        func::cp_copy_dir(in_PATH,out_PATH);
    else
        prt_warning(command.section(' ',0,0));

}

//解析dl命令，并判断输入格式是否规范
void parse_delete(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    QString type=cmd_list.value(1);
    if(type!="-d"&&type!="-f")
        prt_warning(cmd_list.value(0));
    else if(type=="-d")
        for(int i=2;i<cmd_list.size();i++)
        {
            QString path=cmd_list.value(i);
            QStringList list=path.split('.');
            if(list.length()==1)
                func::dl_delete_dir(cmd_list.value(i));
            else
                prt_warning(cmd_list.value(i));
        }
    else
        for(int i=2;i<cmd_list.size();i++)
        {
            QString path=cmd_list.value(i);
            QStringList list=path.split('.');
            if(list.length()==2)
                func::dl_delete_file(cmd_list.value(i));
            else
                prt_warning(cmd_list.value(i));
        }

}

//解析tp命令，并判断输入格式是否规范
void parse_tp(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()!=2)
        prt_warning(cmd_list.value(0));
    else
        func::tp_display(cmd_list.value(1));
}

//解析tree命令，并判断输入格式是否规范
void parse_tree(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()!=1)
        prt_warning(cmd_list.value(0));
    else
        func::tr_display();
}

//解析att命令，并判断输入格式是否规范
void parse_att(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()!=2)
        prt_warning(cmd_list.value(0));
    else
        func::at_display(cmd_list.value(1));
}

//解析mv命令，并判断输入格式是否规范
void parse_move(QString cmd){

    QStringList cmd_list=cmd.split(' ');

    if(cmd_list.size()!=4)
        prt_warning(cmd_list.value(0));
    else if(cmd_list.value(1)=="-d"||cmd_list.value(1)=="-f")
           if(cmd_list.value(1)=="-d")
            func::mv_move_dir(cmd_list.value(2),cmd_list.value(3));
           else
            func::mv_move_file(cmd_list.value(2),cmd_list.value(3));
         else
            prt_warning(cmd_list.value(0));
}

//解析help命令，并判断输入格式是否规范
void parse_help(QString cmd){

    QStringList cmd_list=cmd.split(' ');

    if(cmd_list.size()==1)
        func::help();
    else if(cmd_list.size()==2)
            func::help(map_find(cmd_list.value(1).toStdString()));
    else
            prt_warning(cmd_list.value(0));
}

//解析in命令，并判断输入格式是否规范
void parse_in(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==3)
        func::in_rewrite(cmd_list.value(1),cmd_list.value(2));
    else
        prt_warning(cmd_list.value(0));
}

//解析name命令，并判断输入格式是否规范
void parse_name(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==3)
        func::name_rename(cmd_list.value(1),cmd_list.value(2));
    else
        prt_warning(cmd_list.value(0));

}

//解析find命令，并判断输入格式是否规范
void parse_find(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()==2)
        func::ff(cmd_list.value(1));
    else
        prt_warning(cmd_list.value(0));
}

//解析map命令，并判断输入格式是否规范
void parse_map(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()!=2)
        prt_warning(cmd_list.value(0));
    else
        func::mp_display(cmd_list.value(1));
}

//解析cd命令，并判断输入格式是否规范
void parse_cd(QString cmd){
    QStringList cmd_list=cmd.split(' ');
    if(cmd_list.size()!=2)
        prt_warning(cmd_list.value(0));
    else
        func::cd_change_dir(cmd_list.value(1));

}


//解析命令 进行特判
void parse_cmd(QString cmd){
     std::string command=cmd.section(' ',0,0).toStdString();
     int map_value=map_find(command);
     switch(map_value){
     //create
     case 1: parse_create(cmd); break;
     //mount
     case 2:parse_mount(cmd); break;
     //fmt
     case 3:parse_format(cmd);break;
     //dr
     case 4:parse_dr(cmd);break;
     //cp
     case 5:parse_cp(cmd);break;
     //dl
     case 6: parse_delete(cmd);break;
     //tp
     case 7:parse_tp(cmd); break;
     //att
     case 8: parse_att(cmd);break;
     //mv
     case 9:parse_move(cmd);break;
     //find
     case 10:parse_find(cmd);break;
     //help
     case 11:parse_help(cmd);break;
     //map
     case 12:parse_map(cmd);break;
     //tree
     case 13:parse_tree(cmd);break;
     //close
     case 14:parse_close(cmd);break;
     //in
     case 15:parse_in(cmd);break;
     //mk
     case 16:parse_make(cmd);break;
     //cd
     case 17:parse_cd(cmd);break;
     //name
     case 18:parse_name(cmd);break;
     //unmount
     case 19:parse_unmount(cmd);break;
     default:
         std::cout<<"the command is not fund!"<<std::endl;
     }
}

//用户界面
void user_interface(){
    std::cout<<BOLDGREEN<<"##############################################################\n"
                          "#               欢迎使用Platelet文件系统                     #\n"
                          "#                                                            #\n"
                          "#                                   /＼7　　　 ∠＿/          #\n"
                          "#                                   /　│　　 ／　／          #\n"
                          "#                                  │　Z ＿,＜　／　　 /`ヽ   #\n"
                          "#                                  │　　　　　ヽ　　 /　　〉 #\n"
                          "#                                   Y　　　　　`　 /　　/    #\n"
                          "#                                  ｲ●　､　●　　⊂⊃〈　　/     #\n"
                          "#                                  ()　 へ　　　　|　＼〈    #\n"
                          "#                                   ｰ ､_　 ィ　 │ ／／       #\n"
                          "#                                   / へ　　 /　ﾉ＜| ＼＼    #\n"
                          "#                                   ヽ_ﾉ　　(_／　 │／／     #\n"
                          "#                                   7　　　　　　　|／       #\n"
                          "#                                   ＞―r￣￣`ｰ―＿            #\n"
                          "##############################################################"<<WHITE<<std::endl;
    map_cmd=create_map();
    while(1){
        try{
        std::cout<<"MINI_FS>";
        QString now_path=func::where_path();
        std::cout<<BLUE<<now_path.toStdString()<<WHITE<<"$:";
        std::string command;
        getline(std::cin,command);
        QString Qcommand=QString::fromStdString(command);
        parse_cmd(Qcommand);
        }catch(QString msq){
            qDebug()<<msq;
        }
    }
}

//*-------------------辅助函数----------------------*


//打印警告
void prt_warning(QString command){
    std::cout<<GREEN<<"the "<<command.toStdString()<<" format is wrong"<<WHITE<<std::endl;
}
}
