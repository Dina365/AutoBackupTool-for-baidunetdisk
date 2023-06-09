﻿#include "unbackupfilelist.h"

UnbackupFileList::UnbackupFileList():
    filelist()
{

}
void UnbackupFileList::addFile2target(const QFileInfo &file,const QString &target){
    filelist.push_back(File2target(file,target));
}

QList<File2target>::iterator UnbackupFileList::deleteFile2target(QList<File2target>::iterator it){
    return filelist.erase(it);
}
void UnbackupFileList::deleteFirst(){
    if(!filelist.isEmpty()) filelist.removeFirst();
}
void UnbackupFileList::deleteLast(){
    if(!filelist.isEmpty()) filelist.removeLast();
}

QList<File2target>::iterator UnbackupFileList::item(int n){
    if(n>=0&&n<filelist.size()){
        QList<File2target>::iterator pos=filelist.begin();
        for(int i=0;i<n;i++){
            pos++;
        }
        return pos;
    }else return filelist.end();
}
QList<File2target>::iterator UnbackupFileList::begin(){
    return filelist.begin();
}

QList<File2target>::iterator UnbackupFileList::end(){
    return filelist.end();
}
int UnbackupFileList::size(){
    return filelist.size();
}

void UnbackupFileList::clear(){
    filelist.clear();
}
