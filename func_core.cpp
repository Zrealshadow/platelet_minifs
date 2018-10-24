#include "func_core.h"
#include <iostream>
#include <QDebug>
#include <QString>
//*------------------功能层内部参数------------------------*
namespace _func{
fs::MiniFSGroup init_g;
fs::FSBase *init_pfs=nullptr;
QString path;
}

namespace func {
//*-----------------函数辅助变量---------------------------*

//find命令辅助变量
QList<QString> path_list;
//ocp_copy_file
bool f;
QString path;
//*-----------------功能模块函数---------------------------*


//创建文件系统，并命名为name
void create(QString PATH)
{
    fs::create_minifs(PATH,fs::ONE_GB);
//    _func::path=PATH;
    std::cout<<"create success!"<<std::endl;
}

//挂载name文件系统
void mount(QString PATH,QString name)
{
    _func::init_g=fs::MiniFSGroup::GetInstance();
    _func::init_g.mount(PATH,name);
    _func::init_pfs=_func::init_g.get_fs(name);
    std::cout<<"mount"<<" "<<name.toStdString()<<" "<<"success"<<std::endl;
}

void mount_display()
{
    QList<QPair<int,QString>> list;
    list=_func::init_g.list_fs();
    for(int i=0;i<list.size();i++){
        QPair<int,QString> file_sys=list.at(i);
        int id=file_sys.first;
        QString name=file_sys.second;
        std::cout<<GREEN<<id<<"\t"<<name.toStdString()<<"\t"<<WHITE<<std::endl;
    }
}

void unmount(QString name){
    _func::init_g.unmount(name);
    _func::init_pfs=nullptr;
    std::cout<<"unmount"<<" "<<name.toStdString()<<" "<<"success"<<std::endl;
}
//卸载name文件系统

//格式化当前文件系统
void fm(QString path)
{
    fs::format_minifs(path);
    std::cout<<"format success!"<<std::endl;
}

//关闭当前文件系统
void close(){
    exit(0);
}

//mk 新建操作 ,创建成功，打印提示消息，创建失败，打印对应error
void mk_create_file(QString name)
{
    fs::create_file(_func::init_pfs,name);
}
void mk_create_dir(QString name)
{
    fs::create_dir(_func::init_pfs,name);
}

//dr 打印操作，打印出当前（或所选）目录下的所有文件
void dr_prt_file(QString PATH)
{

    QStringList list=fs::list_dir(_func::init_pfs,PATH);
    list.removeOne(".");
    list.removeOne("..");
    QStringList file_list=list.filter(".");
    int j=0;
    for (int i = 0; i < list.size(); ++i){

        if(file_list.contains(list.value(i)))
            continue;
        else{

            prt_dir(list.value(i));
            if((j+1)%MAX_DR_COL==0)
                std::cout<<std::endl;
            else
                std::cout<<"\t";
            j++;
        }
    }
    for(int i=0;i<file_list.size();++i){
        prt_file(file_list.value(i));
        if((j+1)%MAX_DR_COL==0)
            std::cout<<std::endl;
        else
            std::cout<<"\t";
        j++;

    }
    std::cout<<WHITE<<std::endl;
}

//cp 复制操作，复制成功，打印提示消息，复制失败，打印对应error`
void cp_copy_file(QString in_PATH,QString out_PATH)
{
    int64 len1=in_PATH.length();
    int64 len2=out_PATH.length();
    if(in_PATH.at(0)=='('  && in_PATH.at(len1-1)==')')
    {
        int len=in_PATH.length();
        in_PATH.remove(len-1,1);
        in_PATH.remove(0,1);
        icp_copy_file(in_PATH,out_PATH);
    }
    else if(out_PATH.at(0)=='(' && out_PATH.at(len2-1)==')')
    {
        int len=out_PATH.length();
        out_PATH.remove(len-1,1);
        out_PATH.remove(0,1);
        ocp_copy_file(in_PATH,out_PATH);
    }
    else{
        cp_sys_copy_file(in_PATH,out_PATH);
    }

}



//cp 复制操作，复制成功，打印提示消息，复制失败，打印对应error`
void cp_copy_dir(QString in_PATH,QString out_PATH){
    f=false;
    cp_sys_copy_dir(in_PATH,out_PATH);
}



//dl 删除操作，删除成功，打印提示消息，删除失败，打印对应error
void dl_delete_file(QString PATH)
{
    fs::remove(_func::init_pfs,PATH);
}
void dl_delete_dir(QString PATH)
{
    fs::remove(_func::init_pfs,PATH);
}

//tp 展示操作 打印文件内容
void tp_display(QString PATH){
    fs::FSFile* f=fs::open(_func::init_pfs,PATH,fs::FSFile::OpenMode::R);
    int64 num=0;
    while(1)
    {
        char data[1000000]="";
        int64 len;
        len=fs::read(f,data,num,40960);
        //        std::cout<<data<<std::endl;
        //        std::cout<<"num:"<<num<<std::endl;
        //        std::cout<<"len:"<<len<<std::endl;
        if(len>0)
        {
            std::cout<<data;
            num=num+len;
        }
        else
        {
            break;
        }
    }
    std::cout<<std::endl;
    fs::close(f);
}

//in 写入操作  重写文件内容
void in_rewrite(QString PATH,QString message){
    fs::FSFile* f=fs::open(_func::init_pfs,PATH);
    int64 num=0;
    while(1)
    {
        char data[1000000]="";
        QString mess=message.mid(num,40960);
        strcpy(data,mess.toUtf8().data());
        int64 len=strlen(data);
        if(len!=0)
        {
            fs::write(f,data,num,len);
            num=num+len;
        }
        else
        {
            break;
        }
    }
    fs::close(f);
}




//tree 树型打印出当前目录下所有子目录和文件
//调用了tr_dfs辅助函数
void tr_display()
{
    QList<int> l;
    tree_dfs(1,l,".");
}



//att 打印文件属性
void at_display(QString PATH)
{
    fs::FSFileAttr file_attr=fs::get_file_attr(_func::init_pfs,PATH);
    QString create_time=QDateTime::fromTime_t(file_attr.create_time).toString(Qt::SystemLocaleLongDate);
    QString change_time=QDateTime::fromTime_t(file_attr.change_time).toString(Qt::SystemLocaleLongDate);

    std::cout<<GREEN;
    std::cout<<"文件类型:\t"<<file_attr.file_type<<'\t'<<std::endl;
    std::cout<<"使用块数:\t"<<file_attr.block_num<<'\t'<<std::endl;
    std::cout<<"文件大小:\t"<<file_attr.size<<'\t'<<std::endl;
    std::cout<<"创建时间:\t"<<create_time.toStdString()<<'\t'<<std::endl;
    std::cout<<"最近修改:\t"<<change_time.toStdString()<<'\t'<<std::endl;
    std::cout<<WHITE;
}

//map 展示文件系统下所利用的block
void mp_display(QString PATH)
{
    QList<index_t> block_list=fs::get_block_id_list(_func::init_pfs,PATH);
    std::cout<<GREEN;
    int i;
    for( i=1;i<=block_list.size();i++){
        if(i%MAX_MP_COL!=0)
            std::cout<<block_list.at(i-1)<<"\t";
        else
            std::cout<<block_list.at(i-1)<<"\t"<<std::endl;
    }
    if((i-1)%MAX_MP_COL!=0)
        std::cout<<std::endl;
    std::cout<<"*-----------------------------------------------------------*"<<std::endl;

    index_t rest_block=fs::get_rest_block_num(_func::init_pfs);
    index_t rest_inode=fs::get_rest_inode_num(_func::init_pfs);
    std::cout<<"系统剩余INODE数量:\t"<<rest_inode<<"\t"<<std::endl;
    std::cout<<"系统剩余BLOCK数量:\t"<<rest_block<<"\t"<<std::endl;
    std::cout<<WHITE;
}

//mv 移动操作，移动成功，打印提示消息，移动失败，打印对应error
void mv_move_file(QString in_PATH,QString out_PATH)
{
    fs::move(_func::init_pfs,in_PATH,out_PATH);
}
void mv_move_dir(QString in_PATH,QString out_PATH)
{
    fs::move(_func::init_pfs,in_PATH,out_PATH);
}

//cd 切换操作，切换目录成功，切换失败，打印对应error
void cd_change_dir(QString PATH)
{
    fs::change_current_dir(_func::init_pfs,PATH);
}



//find 查询操作，在current路径下查找名为name的文件或文件夹，成功，打印文件路径，失败，打印对应error
//调用ff_dfs()辅助函数
void ff(QString NAME)
{
    path_list.clear();
    ff_dfs(1,NAME,".");
    if(path_list.isEmpty())
        std::cout<<GREEN<<NAME.toStdString()<<" is not fund"<<WHITE<<std::endl;
    else
        for(int i=0;i<path_list.size();i++)
            std::cout<<GREEN<<path_list.at(i).toStdString()<<WHITE<<std::endl;
}

//name 重命名操作
void name_rename(QString former_name,QString current_name){
    fs::move(_func::init_pfs,former_name,current_name);
}

//help 打印帮助信息
void help(int hash)
{
     std::cout<<BOLDGREEN<<"*--------------------------------------help--------------------------------------------*"<<std::endl;
    switch (hash)
    {
    case 1:
        std::cout<<BOLDGREEN<<"create:\tcreate 名称\n\n\t新建文件系统。\n\n\t创建以“名称”为名的文件系统，要求以.minifs为后缀名"<<RESET<<std::endl;break;
    case 2:
        std::cout<<BOLDGREEN<<"mount:\tmount 名称\n\n\t挂载文件系统。\n\n\t挂载相应名称的文件系统，要求以.minifs为后缀名"<<RESET<<std::endl;break;
    case 3:
        std::cout<<BOLDGREEN<<"fmt:\tfmt 名称\n\n\t格式化文件系统。\n\n\t格式化相应名称的文件系统，要求以.minifs为后缀名"<<RESET<<std::endl;break;
    case 4:
        std::cout<<BOLDGREEN<<"dr:\tdr\n\n\t列出当前目录下文件。\n\n\t列出当前目录下文件,蓝色为文件夹，绿色为文件"<<RESET<<std::endl;break;
    case 5:
        std::cout<<BOLDGREEN<<"cp:\tcp [-d|-f] 路径名 路径名\n\n\t复制文件、文件夹。\n\n\t-d为复制文件夹，-f为复制文件,将前一个路径的文件、文件夹复制到后一个路径的文件、文件夹。\n\t()内为绝对路径"<<RESET<<std::endl;break;
    case 6:
        std::cout<<BOLDGREEN<<"dl:\tdl [-d|-f] [路径名......]\n\n\t删除文件、文件夹。\n\n\t-d为删除文件夹，-f为删除文件,删除路径下的文件、文件夹,可同时删除多个"<<RESET<<std::endl;break;
    case 7:
        std::cout<<BOLDGREEN<<"tp:\ttp 名称\n\n\t显示文件内容。\n\n\t显示相应名称文件的内容"<<RESET<<std::endl;break;
    case 8:
        std::cout<<BOLDGREEN<<"att:\tatt 名称\n\n\t显示文件属性。\n\n\t显示相应名称文件的属性\n文件类型、使用块数、文件大小、创建时间、修改时间等"<<RESET<<std::endl;break;
    case 9:
        std::cout<<BOLDGREEN<<"mv:\tmv [-d|-f] 路径名 路径名\n\n\t移动文件、文件夹。\n\n\t-d为文件夹操作，-f为文件操作，将文件、文件夹从前一个路径移至后一个路径"<<RESET<<std::endl;break;
    case 10:
        std::cout<<BOLDGREEN<<"find:\tfind 名称\n\n\t查找文件，显示文件路径\n\n\t查找系统中所有包含输入的名称字段的文件"<<RESET<<std::endl;break;
    case 11:
        std::cout<<BOLDGREEN<<"help:\thelp [名称]\n\n\t显示帮助内容\n\n\t显示帮助内容,若有参数，则显示相应参数的详细帮助信息"<<RESET<<std::endl;break;
    case 12:
        std::cout<<BOLDGREEN<<"map:\tmap [名称]\n\n\t显示文件在存储空间中所占的块号\n\n\t显示相应名称文件在存储空间中所占的块号"<<RESET<<std::endl;break;
    case 13:
        std::cout<<BOLDGREEN<<"tree:\ttree\n\n\t显示文件系统文件的树状结构"<<RESET<<std::endl;break;
    case 14:
        std::cout<<BOLDGREEN<<"close:\tclose\n\n\t关闭文件系统"<<RESET<<std::endl;break;
    case 15:
        std::cout<<BOLDGREEN<<"in:\tin 名称 内容\n\n\t在文件中输入内容\n\n\t在相应文件中写入相应内容"<<RESET<<std::endl;break;
    case 16:
        std::cout<<BOLDGREEN<<"mk:\tmk [-d|-f] [名称......|路径名]\n\n\t新建文件、文件夹\n\n\t-d为文件夹操作，-f为文件操作，在相应路径创建相应名称的文件或文件夹,可同时创建多个,文件夹没有后缀名，文件必须有后缀名"<<RESET<<std::endl;break;
    case 17:
        std::cout<<BOLDGREEN<<"cd:\tcd [路径名]\n\n\t进入文件夹\n\n\t进入相应路径名的文件夹"<<RESET<<std::endl;break;
    case 18:
        std::cout<<BOLDGREEN<<"name:\tname [-d|-f] 名称 名称\n\n\t重命名\n\n\t-d为文件夹操作，-f为文件操作，将前一个文件、文件夹重命名为后一个"<<RESET<<std::endl;break;
    case 19:
        std::cout<<BOLDGREEN<<"unmount:\tunmount 名称\n\n\t卸载文件系统\n\n\t卸载相应名称的文件系统，要求名称以.mnifs为文件尾"<<RESET<<std::endl;break;
    }
    std::cout<<BOLDGREEN<<"*--------------------------------------------------------------------------------------*"<<std::endl;
    std::cout<<WHITE;
}

void help()
{
    std::cout<<"输入 `help 名称' 以得到有关函数`名称'的更多信息。"<<std::endl;
    std::cout<<BOLDGREEN<<"*--------------------------------------help--------------------------------------------*"<<std::endl;
    std::cout<<std::endl;
    std::cout<<BOLDGREEN<<"create\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"mount\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"unmount\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"fmt\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"dr\t"<<RESET<<"[路径]"<<std::endl;
    std::cout<<BOLDGREEN<<"cp\t"<<RESET<<" [-d|df] 路径名 路径名"<<std::endl;
    std::cout<<BOLDGREEN<<"dl\t"<<RESET<<" [-d|df] [路径名......]"<<std::endl;
    std::cout<<BOLDGREEN<<"in\t"<<RESET<<" 名称 内容"<<std::endl;
    std::cout<<BOLDGREEN<<"tp\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"att\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"mv\t"<<RESET<<" [-d|df] 路径名 路径名"<<std::endl;
    std::cout<<BOLDGREEN<<"find\t"<<RESET<<" 名称"<<std::endl;
    std::cout<<BOLDGREEN<<"help\t"<<RESET<<" [名称]"<<std::endl;
    std::cout<<BOLDGREEN<<"map\t"<<RESET<<" [名称]"<<std::endl;
    std::cout<<BOLDGREEN<<"tree\t"<<RESET<<"无"<<std::endl;
    std::cout<<BOLDGREEN<<"close\t"<<RESET<<"无"<<std::endl;
    std::cout<<BOLDGREEN<<"mk\t"<<RESET<<" [-d|-f] [名称......]"<<std::endl;
    std::cout<<BOLDGREEN<<"cd\t"<<RESET<<"无"<<std::endl;
    std::cout<<BOLDGREEN<<"name\t"<<RESET<<" [-d|df] 路径名 路径名"<<std::endl;
    std::cout<<BOLDGREEN<<std::endl<<"*--------------------------------------------------------------------------------------*"<<std::endl;
    std::cout<<WHITE;
}



//*-----------------功能实现辅助函数-------------------------*

//返回当前文件夹路径 便于显示
QString where_path()
{
    QString path;
    if(_func::init_pfs==nullptr)
        path=QString("");
    else
        path="~"+fs::get_current_dir(_func::init_pfs);
    return path;
}

//文件夹 BOLDBLUE 打印
void prt_dir(QString name){
    std::cout<<BOLDBLUE<<name.toStdString()<<WHITE;
}

//文件 GREEN 打印
void prt_file(QString name){
    std::cout<<GREEN<<name.toStdString()<<WHITE;
}

//find 功能辅助函数 递归函数
void ff_dfs(int deep,QString NAME,QString current_path){
    QStringList all_list=fs::list_dir(_func::init_pfs,current_path);
    all_list.removeOne(".");
    all_list.removeOne("..");
    for(int i=0;i<all_list.size();i++){
        QString name=all_list.value(i);
        QString next_path=current_path+"/"+name;
        char type=fs::get_file_attr(_func::init_pfs,next_path).file_type;
        if(name.contains(NAME,Qt::CaseInsensitive))
            path_list.append(fs::get_current_dir(_func::init_pfs)+next_path.remove("./"));
        else if(type=='d')
            ff_dfs(deep+1,NAME,next_path);

    }
}

    //cp 复制操作，复制成功，打印提示消息，复制失败，打印对应error`
void icp_copy_file(QString in_PATH, QString out_PATH){

        fs::create_file(_func::init_pfs,out_PATH);
        QFile file;
        file.setFileName(in_PATH);
        file.open(QIODevice::ReadOnly);
        if(file.size()>500*fs::ONE_MB)
        {

            std::cout<<GREEN<<"文件大于500M，禁止拷贝"<<WHITE<<std::endl;
            return ;
        }
        fs::FSFile* f=fs::open(_func::init_pfs,out_PATH);
        int64 num=0;
        while(1)
        {
            int64 len;
            char buf[1000000]="";
            len=file.read(buf,40960);
            if(len>0)
            {
                fs::write(f,buf,num,len);
                num=num+len;
            }
            else
            {
                break;
            }
        }
        fs::close(f);
        file.close();
}

    //cp 复制操作，复制成功，打印提示消息，复制失败，打印对应error`
void ocp_copy_file(QString in_PATH, QString out_PATH)
{
        qDebug()<<out_PATH;
        QFile file;
        file.setFileName(out_PATH);
        file.open(QIODevice::WriteOnly);

        fs::FSFile* f=fs::open(_func::init_pfs,in_PATH,fs::FSFile::OpenMode::R);
        int64 num=0 ;
        while(1)
        {
            char data[1000000]="";
            int64 len;
            len=fs::read(f,data,num,40960);
            if(len>0)
            {
                file.write(data,len);
                num=num+len;
            }
            else
            {
                break;
            }
        }
        fs::close(f);
        file.close();
}


void cp_sys_copy_file(QString in_PATH,QString out_PATH)
{
    QStringList o=out_PATH.split('/');
    QString name=out_PATH.section("/",-1,-1);
    int lo=o.length();
    QString out_path;
    for(int i=0 ; i<lo-1 ; i++)
    {
        if(i==0)
        {

            out_path=o.at(i);
        }
        else
        {
            out_path+='/';
            out_path+=o.at(i);
        }
    }
    fs::FSFile* in_f=fs::open(_func::init_pfs,in_PATH,fs::FSFile::OpenMode::R);
    QString in_path=where_path();
    fs::change_current_dir(_func::init_pfs,out_path);
    fs::create_file(_func::init_pfs,name);
    fs::FSFile* out_f=fs::open(_func::init_pfs,name);
    in_path.remove(0,1);
    fs::change_current_dir(_func::init_pfs,in_path);
    int64 num=0;
    while(1)
    {
        char data[1000000]="";
        int64 len;
        len=fs::read(in_f,data,num,40960);
        if(len>0)
        {
            fs::write(out_f,data,num,40960);
            num=num+len;
        }
        else
        {
            break;
        }
    }
    fs::close(in_f);
    fs::close(out_f);
}



//tree 功能辅助函数 递归函数
void tree_dfs(int deep,QList<int>is_last,QString current_path){

    QStringList all_list=fs::list_dir(_func::init_pfs,current_path);
    all_list.removeOne(".");
    all_list.removeOne("..");

    for(int i=0;i<all_list.size();i++){
        QString name=all_list.value(i);
        QString next_path=current_path+"/"+name;
        char type=fs::get_file_attr(_func::init_pfs,next_path).file_type;
        for(int j=0;j<deep-1;j++)
            if(is_last.at(j)!=1)
                std::cout<<"|   ";
            else
                std::cout<<"    ";
        std::cout<<"|---";
        // dir
        if(type=='d'){
            prt_dir(name);
            std::cout<<std::endl;
            if(i==all_list.size()-1)
                is_last.append(1);
            else
                is_last.append(0);
            tree_dfs(deep+1,is_last,next_path);
            is_last.pop_back();

        }
        //file
        else{
            prt_file(name);
            std::cout<<std::endl;
        }
    }
}

void cp_sys_copy_dir(QString in_PATH,QString out_PATH)
{
    if(!f){
        QString na=out_PATH.section('/',-1,-1);
        QString name=out_PATH;
        name.remove(-na.length(),na.length());
        int8 len1=fs::get_dir_layer(_func::init_pfs,name);
        int8 len2=fs::get_tree_layer(_func::init_pfs,in_PATH);
        if((len1+len2+1)>5)
        {
            std::cout<<GREEN<<"不能移动，移动后目录层级数大于5"<<WHITE<<std::endl;
            return ;
        }
        path=where_path();
        path.remove(0,1);
        f=true;
    }
    QString name1=in_PATH.section('/',-1,-1);
    QString name2=out_PATH.section('/',-1,-1);

    QString op=out_PATH;
    op.remove(-name2.length(),name2.length());

    fs::change_current_dir(_func::init_pfs,op);
    fs::create_dir(_func::init_pfs,name2);
    fs::change_current_dir(_func::init_pfs,path);

    QStringList list=fs::list_dir(_func::init_pfs,in_PATH);

    if(list.length()<=2) return ;
    for(int i=2 ; i<list.length();i++)
    {
        QString name=list.at(i);
        QStringList lt=name.split('.');

        if(lt.length()==1)
        {
            cp_sys_copy_dir(in_PATH+'/'+name,out_PATH+'/'+name);
        }
        else
        {
            fs::change_current_dir(_func::init_pfs,path);
            cp_sys_copy_file(in_PATH+'/'+name,out_PATH+'/'+name);
        }
    }
}
}
