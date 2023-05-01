#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setFixedSize(width(),height());
    setWindowIcon(QIcon(ICON));
    QStringList label_ul={"待备份文件","源路径"};
    ui->table_unbackup_list->setHorizontalHeaderLabels(label_ul);
    QStringList label_fp={"文件路径","目标路径"};
    ui->TableFilePath->setHorizontalHeaderLabels(label_fp);

    timer_autorun = new QTimer(this);

    cmd = new QProcess(this);

    //以下为获取cpu线程数
    QProcess cpuInfo;
    cpuInfo.start("wmic", QStringList() << "cpu" << "get" << "NumberOfLogicalProcessors");
    cpuInfo.waitForFinished();
    QString threadsInfo=QString::fromLocal8Bit(cpuInfo.readAllStandardOutput());
    int a,b;
    for(a=0;a<threadsInfo.length();a++){
        if(threadsInfo.at(a)>="0"&&threadsInfo.at(a)<="9")break;
    }
    for(b=a;b<threadsInfo.length();b++){
        if(threadsInfo.at(b)<"0"||threadsInfo.at(b)>"9")break;
    }
    bool ok=false;
    cpuThreads=threadsInfo.midRef(a,b-a).toInt(&ok);
    if(!ok){
        cpuThreads=-1;
        ui->slider_threads->setEnabled(false);
    }else{
        ui->label_num_threads->setText(QString::number(cpuThreads));
        cmdThreads=cpuThreads;
        ui->slider_threads->setMaximum(cpuThreads);
        ui->slider_threads->setMinimum(1);
        ui->slider_threads->setValue(cpuThreads);
    }
    //获取失败则赋值为-1

    bkedlist = new FileList();

    unbkFilelist=new UnbackupFileList();
    updatedlist=new FileList();
    initBkedlistFromFile();
    initUpdlistFromFile();
    initSource2TargetFromFile();
    //这里写从文件中初始化参数
    ConfigInitial();
    if(configInited)ui->Btn_ensure_settings->setEnabled(false);
    target=unbkFilelist->begin();



    qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    connect(cmd,SIGNAL(finished(int,QProcess::ExitStatus)),this,SLOT(slotCmdFinished(int,QProcess::ExitStatus)),Qt::QueuedConnection);
    connect(this,SIGNAL(startAutoDelete()),this,SLOT(on_startAutoDelete()),Qt::QueuedConnection);
    connect(this,SIGNAL(finishedAutoDelete(bool)),this,SLOT(on_finishedAutoDelete(bool)),Qt::QueuedConnection);
    connect(this->timer_autorun,SIGNAL(timeout()),this,SLOT(slot_timer_timeout()));

    if(needClickAutorun){
       emit ui->btn_autorun->clicked();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete Target_Path;
    delete password;
    delete bz_path;
    delete unbkFilelist;
    delete cmd;
    delete bdyunpath;
    delete bkedlist;
    delete updatedlist;
}

void MainWindow::initBkedlistFromFile(){
    if(!QFileInfo::exists(BKEDLIST))return;
    QFile fin(BKEDLIST);
    fin.open(QFile::ReadOnly|QFile::Text);
    if(!fin.isOpen())return;
    int num=QString::fromUtf8(fin.readLine()).toInt();
//    qDebug()<<num<<endl;
    for(int i=0;i<num;i++){
        QString data;
        QStringList s;
        while(s.size()<3)
        {
            data=QString::fromUtf8(fin.readLine());
            data.remove('\n');
            s=data.split('|');
        }

        bkedlist->addFile_datetime(s[0],s[1],s[2]);
    }
    fin.close();
}
void MainWindow::initUpdlistFromFile(){
    if(!QFileInfo::exists(UPDATELIST))return;
    QFile fin(UPDATELIST);
    fin.open(QFile::ReadOnly|QFile::Text);
    if(!fin.isOpen())return;
    int num=QString::fromUtf8(fin.readLine()).toInt();
//    qDebug()<<num<<endl;
    for(int i=0;i<num;i++){
        QString data;
        QStringList s;
        while(s.size()<3)
        {
            data=QString::fromUtf8(fin.readLine());
            data.remove('\n');
            s=data.split('|');
        }

        updatedlist->addFile_datetime(s[0],s[1],s[2]);
    }
    fin.close();
}

void MainWindow::saveBkedlistToFile(){
    if(bkedlist->size()<=0)return;

    QFile fout(BKEDLIST);
    fout.open(QFile::WriteOnly|QFile::Text);
    if(!fout.isOpen()){
        return;/*
        qDebug()<<"文件打开失败";*/
    }
    fout.write((QString::number(bkedlist->size())+"\n").toUtf8());
    for(QList<file_datetime>::iterator it=bkedlist->begin();it!=bkedlist->end();it++){
        fout.write((it->file+"|"+it->date+"|"+it->time+"\n").toUtf8());
    }/*
    qDebug()<<bkedlist->size();*/
    fout.close();
}
void MainWindow::saveUpdlistToFile(){

    QFile fout(UPDATELIST);
    fout.open(QFile::WriteOnly|QFile::Text);
    if(!fout.isOpen()){
        return;/*
        qDebug()<<"文件打开失败";*/
    }
    fout.write((QString::number(updatedlist->size())+"\n").toUtf8());
    for(QList<file_datetime>::iterator it=updatedlist->begin();it!=updatedlist->end();it++){
        fout.write((it->file+"|"+it->date+"|"+it->time+"\n").toUtf8());
    }/*
    qDebug()<<bkedlist->size();*/
    fout.close();
}

void MainWindow::deleteBkedlistFile(){
    if(QFileInfo::exists(BKEDLIST)){
        QFile file(BKEDLIST);
        file.remove();
        bkedlist->clear();
    }
}

void MainWindow::filter(){//从bkedlist中查找相同的路径，若找到，则删除unbkFilelist中对应的项。也可以判断修改时间是否较新
    if(!btn_enable_filter)return;
    int i=0,j=0;
    for(QList<File2target>::iterator it=unbkFilelist->begin();i<unbkFilelist->size();){
        j=0;
        bool finded=false;
        for(QList<file_datetime>::iterator p=bkedlist->begin();j<bkedlist->size();){
            if(p->file==it->fileinfo.absoluteFilePath()){
                if(btn_filterByTime){
                    if((p->date < it->fileinfo.lastModified().date().toString("yyyy.MM.dd"))
                        || ((p->date == it->fileinfo.lastModified().date().toString("yyyy.MM.dd"))
                            &&
                            (p->time < it->fileinfo.lastModified().time().toString("hh:mm")))){
                        updatedlist->addFile_datetime(p->file,p->date,p->time);//添加到备份文件更新列表
                        p=bkedlist->deletefile_datetime(p);//比较时间，如果时间较新，则从已备份列表删去这一项，后面不用继续遍历了
                        break;
                    }else{
                        finded=true;//时间较旧，标记为被筛除
                        break;
                    }
                }else{
                    finded=true;//找到相同的路径，标记为被筛除
                    break;
                }
            }
            p++;
            j++;
        }if(finded){
            it=unbkFilelist->deleteFile2target(it);
        }else{
            it++;
            i++;
        }
    }
    if(!btn_filterByBdyun)return;
    i=0;
    if(QFileInfo::exists(*bdyunpath+"/BaiduYunCacheFileV0.db")){
//        qDebug()<<list.size();
        QFile f0(*bdyunpath+"/BaiduYunCacheFileV0.db");
        f0.open(QFile::ReadOnly);
        QByteArray db0;
        if(f0.isOpen())db0 = f0.readAll();
//        qDebug()<<db0.left(30);
//        qDebug()<<f0.size();
        QFile f1(*bdyunpath+"/BaiduYunCacheFileV0.db-wal");
        f1.open(QFile::ReadOnly);
        QByteArray db1;
        if(f1.isOpen())db1 = f1.readAll();

        QFile f2(*bdyunpath+"/BaiduYunCacheFileV0.db-shm");
        f2.open(QFile::ReadOnly);
        QByteArray db2;
        if(f2.isOpen())db2 = f2.readAll();
        for(QList<File2target>::iterator it=unbkFilelist->begin();i<unbkFilelist->size();){
            bool isExist=false;

            if(!isExist&&f0.isOpen()){
                isExist = db0.contains((QFileInfo(it->fileinfo.absolutePath()).baseName()+"/"+filenameEncrypt(it->fileinfo.baseName())+".abt").toUtf8());
            }
            if(!isExist&&f1.isOpen()){
                isExist = db1.contains((QFileInfo(it->fileinfo.absolutePath()).baseName()+"/"+filenameEncrypt(it->fileinfo.baseName())+".abt").toUtf8());
//                qDebug()<<isExist;
            }
            if(!isExist&&f2.isOpen()){
                isExist = db2.contains((QFileInfo(it->fileinfo.absolutePath()).baseName()+"/"+filenameEncrypt(it->fileinfo.baseName())+".abt").toUtf8());
            }
            if(isExist){
                it=unbkFilelist->deleteFile2target(it);
            }else{
//                qDebug()<<(QFileInfo(it->fileinfo.absolutePath()).baseName()+"/"+filenameEncrypt(it->fileinfo.baseName())+".abt");
//                qDebug()<<isExist;
  //            qDebug()<<isExist;
                it++;
                i++;
            }
        }
    }
}

void MainWindow::ConfigInitial(){
    Target_Path=new QString();
    Max_Filesize=0;
    password = new QString();
    bz_path = new QString(BZ_PATH);
    ui->textEdit_bzpath->setText(*bz_path);
    bdyunpath=new QString();
    
    ui->btn_enableFilter->setEnabled(true);
    btn_enable_filter=true;
    ui->btn_enableFilter->setText("已开启");
    if(!btn_enable_filter){
        ui->btn_filterByTime->setEnabled(false);
        ui->btn_filterByBdyun->setEnabled(false);
    }
    btn_filterByTime=false;
    ui->btn_filterByTime->setText("已关闭");
    btn_filterByBdyun=false;
    ui->btn_filterByBdyun->setText("已关闭");

    bool ret = ConfigInitialFromFile();
    if(ret)
    {
        ui->textEdit_targetPath->setText(*Target_Path);
        ui->textEdit_password->setText(*password);
        ui->textEdit_MaxFileSize->setText(QString::number(Max_Filesize));
        ui->textEdit_bdyunpath->setText(*bdyunpath);
        configInited=true;
        Btn_ensure_enable=false;
    }
    else{
        configInited=false;
        Btn_ensure_enable=true;
        ui->Btn_ensure_settings->setEnabled(true);
        ui->btn_autorun->setEnabled(false);
        ui->btn_autoDelete->setEnabled(false);
        ui->Btn_startBackup->setEnabled(false);
        ui->btn_deleteBkedlist->setEnabled(false);
    }

    initThreadsNumFromFile();
}

void MainWindow::closeEvent(QCloseEvent *event){
//    qDebug()<<bkedlist->size();

    //先直接开始保存文件到本地
    saveBkedlistToFile();
    saveUpdlistToFile();
    saveSource2TargetToFile();
    saveConfigToFile();
    saveThreadsNumToFile();
    if(isProcessing||isAutoDeleting){
        QMessageBox::information(this,"关闭提示","当前有任务进程运行中，强行关闭将可能会导致未知错误。","确定");
        event->ignore();
        return;
    }
    int ret = QMessageBox::question(this,"关闭提示","您确定要退出程序吗？配置信息和文件列表将会自动保存到本地。","取消","确定");
    if(ret == 0)/*点击取消按钮*/
    {
        event->ignore();
    }
    else if(ret == 1)/*点击确认按钮*/
    {
        //这里进行文件保存操作

        //最好确认文件保存成功再退出
        event->accept();
    }
}

QString MainWindow::filenameEncrypt(const QString &filename){//文件名加密
    QString name_en;
    for(int i=0;i<filename.length()&&i<51;i++){
        name_en+=char((0xf000&filename[i].unicode())>>12)+'A';
        name_en+=char((0x0f00&filename[i].unicode())>>8)+'A';
        name_en+=char((0x00f0&filename[i].unicode())>>4)+'A';
        name_en+=char((0x000f&filename[i].unicode()))+'A';
    }/*
    qDebug()<<name_en;*/
    return name_en;
}

void MainWindow::bz_process(const File2target& file_target){//调用bandizip进行加密压缩
    QStringList args;
    args<<"c"<<"-aoa"<<QString("-p:")+*password<<"-y";
    if(cpuThreads>0&&cmdThreads>0&&cmdThreads<=cpuThreads)args<<QString("-t:")+QString::number(cmdThreads);
    args<<"-fmt:zip"<<file_target.target_path+"/"+filenameEncrypt(file_target.fileinfo.baseName())+".abt"
            <<file_target.fileinfo.absoluteFilePath();
//    qDebug()<<args;
    cmd->start(*bz_path,args);
    for(int i=0;i<updatedlist->size();i++){
        if(updatedlist->item(i)->file==file_target.fileinfo.absoluteFilePath()){
            updatedlist->item(i)->file = file_target.target_path+"/"+filenameEncrypt(file_target.fileinfo.baseName())+".abt";
            break;
        }
    }
}

void MainWindow::startCmd(){
    if(isProcessing)return;
    isProcessing=true;
    ui->slider_threads->setEnabled(false);
    bz_process(*target);
}

void MainWindow::slotCmdFinished(int exitCode, QProcess::ExitStatus exitStatus){
    isProcessing=false;
    if(cpuThreads>0)ui->slider_threads->setEnabled(true);
//    qDebug()<<exitCode;
    if(exitStatus == QProcess::NormalExit&&exitCode==0){
        if(btn_enable_filter){//开启了过滤器，则不可能与已备份列表重复
            bkedlist->addFile_datetime(target->fileinfo.absoluteFilePath(),
                                       target->fileinfo.lastModified().date().toString("yyyy.MM.dd"),
                                       target->fileinfo.lastModified().time().toString("hh:mm"));
        }else{
            int j=0;
            bool finded=false;
            for(QList<file_datetime>::iterator p=bkedlist->begin();j<bkedlist->size();){
                if(p->file==target->fileinfo.absoluteFilePath()){
                    finded=true;
                    p->date=target->fileinfo.lastModified().date().toString("yyyy.MM.dd");
                    p->time=target->fileinfo.lastModified().time().toString("hh:mm");
                    break;
                }
                p++;
                j++;
            }
            if(!finded){
                bkedlist->addFile_datetime(target->fileinfo.absoluteFilePath(),
                                           target->fileinfo.lastModified().date().toString("yyyy.MM.dd"),
                                           target->fileinfo.lastModified().time().toString("hh:mm"));
            }
        }
        target=unbkFilelist->deleteFile2target(target);
        ui->table_unbackup_list->removeRow(error_num);
        int size=getLocalFileSize();
        if(unbkFilelist->size()>error_num&&size<Max_Filesize&&!mark_stopBtn)startCmd();
        else {
            if(!autorun)
            {
                ui->Btn_startBackup->setEnabled(true);

                ui->btn_enableFilter->setEnabled(true);
                if(btn_enable_filter){
                    ui->btn_filterByTime->setEnabled(true);
                    ui->btn_filterByBdyun->setEnabled(true);
                }

                ui->btn_autoDelete->setEnabled(true);
                ui->btn_deleteBkedlist->setEnabled(true);
            }   ui->Btn_startBackup->setText("开始备份");
            if(mark_stopBtn){
                mark_stopBtn=false;
                ui->btn_stopBackup->setEnabled(true);
            }
            if(!autorun)QMessageBox::information(this,"备份完成",QString("备份完成，共")+QString::number(error_num)+"个文件备份失败。","确认");
            getUnbkedFileSize();
            getLocalFileSize();
        }
    }
    
    else{
        error_num++;
        target++;
        int size=getLocalFileSize();
        if(unbkFilelist->size()>error_num&&size<Max_Filesize&&!mark_stopBtn)startCmd();
        else {
            if(!autorun)
            {
                ui->Btn_startBackup->setEnabled(true);

                ui->btn_enableFilter->setEnabled(true);
                if(btn_enable_filter){
                    ui->btn_filterByTime->setEnabled(true);
                    ui->btn_filterByBdyun->setEnabled(true);
                }
                ui->btn_autoDelete->setEnabled(true);
                ui->btn_deleteBkedlist->setEnabled(true);
            }   ui->Btn_startBackup->setText("开始备份");
            if(mark_stopBtn){
                mark_stopBtn=false;
                ui->btn_stopBackup->setEnabled(true);
            }
            if(!autorun)QMessageBox::information(this,"备份完成",QString("备份完成，共")+QString::number(error_num)+"个文件备份失败。","确认");
            getUnbkedFileSize();
            getLocalFileSize();
        }
    }
}

void MainWindow::getUnbackupFilelist(){
    if(ui->TableFilePath->rowCount()<=0){
        QMessageBox::critical(this,"错误","路径为空，请先设置路径。","确认");
        return;
    }
    unbkFilelist->clear();
    for(int i=0;i<ui->TableFilePath->rowCount();i++){
        QDir dir(ui->TableFilePath->item(i,0)->text());
        if(!dir.exists()){
            QMessageBox::critical(this,"错误","路径有错,请检查。","确认");
            break;
        }else{
            QFileInfoList flist =dir.entryInfoList(QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot);
            for(int j=0;j<flist.length();j++){
                unbkFilelist->addFile2target(flist.at(j),ui->TableFilePath->item(i,1)->text());
            }
        }
    }

    //这里写过滤已备份文件的操作
    if(btn_enable_filter)filter();
    if(unbkFilelist->size()<=0){
        if(!autorun)QMessageBox::information(this,"提示","当前暂无需要备份的文件。","确认");
        return;
    }

    //展示到界面上
    for(int i=ui->table_unbackup_list->rowCount()-1;i>=0;i--){
    ui->table_unbackup_list->removeRow(i);
    }//先清空列表
    QList<File2target>::iterator it=unbkFilelist->begin();
    for(int i=0;i<unbkFilelist->size();i++,it++){
        int r_count=ui->table_unbackup_list->rowCount();
//        qDebug()<<r_count;
        ui->table_unbackup_list->insertRow(r_count);
        QTableWidgetItem *i1=new QTableWidgetItem(it->fileinfo.fileName());
        QTableWidgetItem *i2=new QTableWidgetItem(it->fileinfo.absolutePath());
        ui->table_unbackup_list->setItem(r_count,0,i1);
        ui->table_unbackup_list->setItem(r_count,1,i2);
    }

}

//void MainWindow::mousePressEvent(QMouseEvent *event){
//    if(event->x()<ui->TableFilePath->x() || event->x()>ui->TableFilePath->x()+ui->TableFilePath->width()
//       || event->y()<ui->TableFilePath->y() ||event->y()>ui->TableFilePath->y()+ui->TableFilePath->height())
//    {
//        selected_row_TableFilePath = -1;
//        ui->BtnDeleteFilePath->setEnabled(false);
//    }else return QMainWindow::mousePressEvent(event);
//}

void MainWindow::on_BtnAddFilePath_clicked()
{
    int last_row=ui->TableFilePath->rowCount();
    QString path = QFileDialog::getExistingDirectory(this);
    if(path.isEmpty())return;
    for(int i = 0;i<last_row;i++){
        if(ui->TableFilePath->item(i,0)->text() == path){
            QMessageBox::critical(this,"错误","文件路径已存在，请重新选择。","确认");
            return;
        }
    }if(Target_Path == path){
        QMessageBox::critical(this,"错误","源路径与解压目的路径不能一致，请重新选择。","确认");
        return;
    }
    ui->TableFilePath->insertRow(last_row);
    QTableWidgetItem* item0 = new QTableWidgetItem(path);
    ui->TableFilePath->setItem(last_row,0,item0);
    QString target_path(*Target_Path);
    if(!target_path.endsWith('/'))target_path+="/";
    if(path.contains(':')||path.startsWith('/'))path.remove(0,path.indexOf("/")+1);
    target_path+=path;
    QTableWidgetItem* item1 = new QTableWidgetItem(target_path);
    ui->TableFilePath->setItem(last_row,1,item1);
}



void MainWindow::on_BtnDeleteFilePath_clicked()
{
    QVector<int> list;//带删除行的列表
    for(int i=0;i<ui->TableFilePath->rowCount();i++){
    for(int j=0;j<2;j++){
        if(ui->TableFilePath->item(i,j)->isSelected()){
            list.push_back(i);
            break;
        }
    }
    }
    if(list.isEmpty())return;
    int ret = QMessageBox::question(this,"删除确认","您确定要该删除路径吗？","确定","我手滑了");
    if(ret == 0){
        if(QMessageBox::question(this,"再次确认","您确定真的要该删除路径吗？","我手滑了","再次确定") == 1)
        {
            for(int i=list.length()-1;i>=0;i--){
                ui->TableFilePath->removeRow(list[i]);
            }
        }
    }
}



void MainWindow::on_TableFilePath_cellDoubleClicked(int row, int column)
{
    QTableWidgetItem* item = ui->TableFilePath->item(row,column);
    QString ret;
    ItemEditDialog dia(this,item,&ret);
    if(dia.exec()==QDialog::Accepted){
        if(ret==ui->TableFilePath->item(row,(column==0?1:0))->text()){
            QMessageBox::critical(this,"错误","源目录与解压目的目录不能相同。","确认");
        }
        else if(QFileInfo(ret).isDir()){
            item->setText(ret);
        }
    }

}

//void MainWindow::on_TableFilePath_itemClicked(QTableWidgetItem *item)
//{
//    qDebug()<<item->text()<<endl;
//}

void MainWindow::on_btn_select_path_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this);

    ui->textEdit_targetPath->setText(path);
}

void MainWindow::on_textEdit_targetPath_textChanged()
{
    if(!Btn_ensure_enable) {
        ui->Btn_ensure_settings->setEnabled(true);
        Btn_ensure_enable=true;
    }
}

void MainWindow::on_textEdit_password_textChanged()
{
    if(!Btn_ensure_enable) {
        ui->Btn_ensure_settings->setEnabled(true);
        Btn_ensure_enable=true;
    }
}

void MainWindow::on_textEdit_MaxFileSize_textChanged()
{
    if(!Btn_ensure_enable) {
        ui->Btn_ensure_settings->setEnabled(true);
        Btn_ensure_enable=true;
    }
}

void MainWindow::on_textEdit_bzpath_textChanged()
{
    if(!Btn_ensure_enable) {
        ui->Btn_ensure_settings->setEnabled(true);
        Btn_ensure_enable=true;
    }
}

bool MainWindow::chkcfg(){
    if(configInited)return true;
    else{
        QString error_info;//存储错误信息
        //检查解压目的路径
        bool flag0=true;
        int r_count=ui->TableFilePath->rowCount();
        for(int i = 0;i<r_count;i++){
            if(ui->TableFilePath->item(i,0)->text() == ui->textEdit_targetPath->toPlainText()){
                flag0=false;
                error_info+="备份目的路径：解压目的目录不能与已有的源目录相同。\n";
                break;
            }
        }
        if(!QFileInfo(ui->textEdit_targetPath->toPlainText()).isDir()){
            flag0=false;
            error_info+="备份目的路径：设置的路径不是文件夹或不存在。\n";
        }

        //检查解压密码
        bool flag1=true;
        if(ui->textEdit_password->toPlainText().length()>127){
            flag1=false;
            error_info+="解压密码：解压密码长度不能超过127字符。\n";
        }

        //检查文件阈值
        bool flag2=true;
        ui->textEdit_MaxFileSize->toPlainText().toInt(&flag2);
        if(!flag2){
            error_info+="本地文件阈值：请输入正确的数字。\n";
        }

        //检查bz.exe路径
        bool flag3=true;
        if(!QFileInfo::exists(ui->textEdit_bzpath->toPlainText())){
            flag3=false;
            error_info+="bandizip路径：路径错误，文件不存在。\n";
        }
        //检查百度网盘用户数据路径
        bool flag4=true;
        if(!QFileInfo::exists(ui->textEdit_bdyunpath->toPlainText())){
            flag4=false;
            error_info+="百度网盘用户数据路径：路径错误，文件不存在。\n";
        }
        if(flag0&&flag1&&flag2&&flag3&&flag4){
            configInited=true;
            ui->btn_autorun->setEnabled(true);
            ui->btn_autoDelete->setEnabled(true);
            ui->Btn_startBackup->setEnabled(true);
            ui->btn_deleteBkedlist->setEnabled(true);
            return true;
        }else{
            QMessageBox::critical(this,"错误",error_info,"确认");
            return false;
        }
    }
}

void MainWindow::on_Btn_ensure_settings_clicked()
{   
    if(isProcessing||isAutoDeleting||autorun){
        QMessageBox::critical(this,"错误","有任务正在进行，请等任务结束后再继续。","确认");
        return;
    }
    bool ret=true;
    if(!configInited){
        ret=chkcfg();
    }
    if(!ret)return;

    ui->Btn_ensure_settings->setEnabled(false);
    QString error_info;//存储错误信息
    //检查解压目的路径
    bool flag0=true;
    int r_count=ui->TableFilePath->rowCount();
    for(int i = 0;i<r_count;i++){
        if(ui->TableFilePath->item(i,0)->text() == ui->textEdit_targetPath->toPlainText()){
            flag0=false;
            error_info+="备份目的路径：解压目的目录不能与已有的源目录相同。\n";
            break;
        }
    }
    if(!QFileInfo(ui->textEdit_targetPath->toPlainText()).isDir()){
        flag0=false;
        error_info+="备份目的路径：设置的路径不是文件夹或不存在。\n";
    }

    //检查解压密码
    bool flag1=true;
    if(ui->textEdit_password->toPlainText().length()>127){
        flag1=false;
        error_info+="解压密码：解压密码长度不能超过127字符。\n";
    }

    //检查文件阈值
    bool flag2=true;
    ui->textEdit_MaxFileSize->toPlainText().toInt(&flag2);
    if(!flag2){
        error_info+="本地文件阈值：请输入正确的数字。\n";
    }

    //检查bz.exe路径
    bool flag3=true;
    if(!QFileInfo::exists(ui->textEdit_bzpath->toPlainText())){
        flag3=false;
        error_info+="bandizip路径：路径错误，文件不存在。\n";
    }
    //检查百度网盘用户数据路径
    bool flag4=true;
    if(!QFileInfo::exists(ui->textEdit_bdyunpath->toPlainText())){
        flag4=false;
        error_info+="百度网盘用户数据路径：路径错误，文件不存在。\n";
    }

    //应用设置
    if(flag0)*Target_Path = ui->textEdit_targetPath->toPlainText();
    else ui->textEdit_targetPath->setText(*Target_Path);

    if(flag1)*password = ui->textEdit_password->toPlainText();
    else ui->textEdit_password->setText(*password);

    if(flag2)Max_Filesize=ui->textEdit_MaxFileSize->toPlainText().toInt();
    else ui->textEdit_MaxFileSize->setText(QString::number(Max_Filesize));

    if(flag3)*bz_path = ui->textEdit_bzpath->toPlainText();
    else ui->textEdit_bzpath->setText(*bz_path);

    if(flag4)*bdyunpath = ui->textEdit_bdyunpath->toPlainText();
    else ui->textEdit_bdyunpath->setText(*bdyunpath);

    if(!error_info.isEmpty()){
        QMessageBox::critical(this,"错误",error_info,"确认");
    }
    getLocalFileSize();
    getUnbkedFileSize();
    Btn_ensure_enable=false;
}

void MainWindow::on_btn_select_path_bz_clicked()
{
    QString path = QFileDialog::getOpenFileName(this);

    ui->textEdit_bzpath->setText(path);
}

void MainWindow::on_Btn_startBackup_clicked()
{
    if(isProcessing||isAutoDeleting)return;
    if(getLocalFileSize()>Max_Filesize){
        if(!autorun){
            QMessageBox::critical(this,"超过阈值","备份文件大小已超过阈值，请清理。","确认");
        }return;
    }
    getUnbackupFilelist();
    if(unbkFilelist->size()<=0)return;
    getUnbkedFileSize();
    ui->Btn_startBackup->setText("正在备份中");
    ui->Btn_startBackup->setEnabled(false);
    ui->btn_enableFilter->setEnabled(false);
    ui->btn_filterByTime->setEnabled(false);
    ui->btn_filterByBdyun->setEnabled(false);
    ui->btn_autoDelete->setEnabled(false);
    ui->btn_deleteBkedlist->setEnabled(false);
    error_num=0;
    target=unbkFilelist->begin();

    startCmd();

//    target = unbkFilelist->begin();
//    error_num=0;;
//    while(unbkFilelist->size()>i){
//        if(bz_process(*it)){
//            unbkFilelist->deleteFile2target(it);
//            ui->table_unbackup_list->removeRow(i);
//        }else{
//            it++;
//            i++;
//        }
//    }
//    ui->Btn_startBackup->setEnabled(true);
//    QMessageBox::information(this,"备份完成",QString("备份完成，共")+QString::number(i)+"个文件备份失败。","确认");
}


void MainWindow::on_btn_select_path_bdyun_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this);

    ui->textEdit_bdyunpath->setText(path);
}

void MainWindow::on_textEdit_bdyunpath_textChanged()
{
    if(!Btn_ensure_enable) {
        ui->Btn_ensure_settings->setEnabled(true);
        Btn_ensure_enable=true;
    }
}

void MainWindow::on_btn_enableFilter_clicked()
{
    if(btn_enable_filter){
        ui->btn_enableFilter->setText("已关闭");
        btn_enable_filter=false;
        ui->btn_filterByTime->setEnabled(false);
        ui->btn_filterByBdyun->setEnabled(false);
    }else{
        ui->btn_enableFilter->setText("已开启");
        btn_enable_filter=true;
        ui->btn_filterByTime->setEnabled(true);
        ui->btn_filterByBdyun->setEnabled(true);
    }
}

void MainWindow::on_btn_filterByTime_clicked()
{
    if(btn_filterByTime){
        ui->btn_filterByTime->setText("已关闭");
        btn_filterByTime=false;
    }else{
        ui->btn_filterByTime->setText("已开启");
        btn_filterByTime=true;
    }
}

int MainWindow::getLocalFileSize(){
    int size=int(getDirSize(*Target_Path)/1024/1024);
    ui->label_num_filesize->setText(QString::number(size));
    return size;
}
int MainWindow::getUnbkedFileSize(){
    int size=0;
    if(unbkFilelist->size()<=0)size=0;
    else{
        int i=0;
        for(QList<File2target>::iterator it = unbkFilelist->begin();i<unbkFilelist->size();i++,it++){
            size+=int(getDirSize(it->fileinfo.absoluteFilePath())/1024/1024);
        }
    }
    ui->label_num_unbackup->setText(QString::number(size));
    return size;
}

void MainWindow::on_btn_autoDelete_clicked()
{
    if(isProcessing)return;
    if(isAutoDeleting){
        if(!autorun)QMessageBox::information(this,"进行中","已有进程正在运行中，请等待进程结束。","确认");
        ui->btn_autoDelete->setText("进行中");
        ui->btn_autoDelete->setEnabled(false);
        return;
    }else{
        emit startAutoDelete();
        ui->btn_autoDelete->setText("进行中");
        ui->btn_autoDelete->setEnabled(false);
        return;
    }
}

void MainWindow::getAllAbt(const QString &path, QFileInfoList &list){
    QDir dir(path);
    list.append(dir.entryInfoList(QStringList("*.abt"),QDir::Files|QDir::Writable));
    QFileInfoList child_dirs=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
    for(int i=0;i<child_dirs.size();i++){
        getAllAbt(child_dirs.at(i).absoluteFilePath(),list);
    }
}

void MainWindow::on_startAutoDelete(){
    if(isAutoDeleting)return;
    isAutoDeleting=true;
    if(QFileInfo::exists(*bdyunpath+"/BaiduYunCacheFileV0.db")){
        QFileInfoList list;
        getAllAbt(*Target_Path,list);
        int ret = 1;
        if(autorun){
            ret = 1;
        }else if(updatedlist->size()<=0){
            ret = 0;
        }
        else{
            ret = QMessageBox::question(this,"提示","自动删除不会检测文件更新后重备份的文件是否已上传，是否删除这类文件?(除非确认已全部上传，否则请点否)","是","否");
            if(ret==0){
                ret= QMessageBox::question(this,"再次确认","自动删除不会检测文件更新后重备份的文件是否已上传，除非确认已全部上传，否则请点否。","是","否");
            }
        }

        if(ret==1){
            for(int i=0;i<list.size();i++){
                for(int j=0;j<updatedlist->size();j++){
                    if(list.at(i).absoluteFilePath()==updatedlist->item(j)->file){
                        list.removeAt(i);
                        i--;
                        break;
                    }
                }
            }
        }else{
            updatedlist->clear();
        }

//        qDebug()<<list.size();
        QFile f0(*bdyunpath+"/BaiduYunCacheFileV0.db");
        f0.open(QFile::ReadOnly);
        QByteArray db0;
        if(f0.isOpen())db0 = f0.readAll();
//        qDebug()<<db0.left(30);
//        qDebug()<<f0.size();
        QFile f1(*bdyunpath+"/BaiduYunCacheFileV0.db-wal");
        f1.open(QFile::ReadOnly);
        QByteArray db1;
        if(f1.isOpen())db1 = f1.readAll();

        QFile f2(*bdyunpath+"/BaiduYunCacheFileV0.db-shm");
        f2.open(QFile::ReadOnly);
        QByteArray db2;
        if(f2.isOpen())db2 = f2.readAll();
        bool* needRemove = new bool[list.size()]{false};
        for(int i=0;i<list.size();i++){
            bool isExist=false;

            if(!isExist&&f0.isOpen()){
                isExist = db0.contains((QFileInfo(list.at(i).absolutePath()).baseName()+"/"+list.at(i).fileName()).toUtf8());
            }
            if(!isExist&&f1.isOpen()){
                isExist = db1.contains((QFileInfo(list.at(i).absolutePath()).baseName()+"/"+list.at(i).fileName()).toUtf8());
//                qDebug()<<isExist;
            }
            if(!isExist&&f2.isOpen()){
                isExist = db2.contains((QFileInfo(list.at(i).absolutePath()).baseName()+"/"+list.at(i).fileName()).toUtf8());
            } /*qDebug()<<(QFileInfo(list.at(i).absolutePath()).baseName()+"/"+list.at(i).fileName()).toUtf8();
              qDebug()<<isExist;*/
//            qDebug()<<isExist;
            needRemove[i]=isExist;
        }

        for(int i=0;i<list.size();i++){
            if(needRemove[i]){
                QFile f(list.at(i).absoluteFilePath());
                f.remove();
            }
        }
        delete[] needRemove;
        emit finishedAutoDelete(true);
    }else{
        emit finishedAutoDelete(false);
    }

}

void MainWindow::on_finishedAutoDelete(bool code){
    if(!code){
       if(!autorun) QMessageBox::critical(this,"错误","百度网盘用户数据路径不正确，请设置一个正确的路径。","确认");
    }else{
        if(!autorun)QMessageBox::information(this,"消息","删除已完成。","确认");
    }
    if(!autorun)ui->btn_autoDelete->setEnabled(true);
    ui->btn_autoDelete->setText("开始运行");
    isAutoDeleting=false;
    getLocalFileSize();
    if(autorun)emit ui->Btn_startBackup->clicked();
}

QString MainWindow::filenameDecrypt(const QString &filename){
    QFileInfo file(filename);
    if(file.completeSuffix()!="abt")return QString();
    QString ret;
    QString basename=file.baseName();
    for(int i=0;i<basename.size()/4;i++){
        ushort c=0x0000;
        c=c|((ushort(basename[i*4].toLatin1()-'A'))<<12)|((ushort(basename[i*4+1].toLatin1()-'A'))<<8)
            |((ushort(basename[i*4+2].toLatin1()-'A'))<<4)|(ushort(basename[i*4+3].toLatin1()-'A'));
        ret+=QChar(c);
    }
    return ret+".zip";
}

void MainWindow::on_btn_filenameDecrypt_clicked()
{
    QString path = QFileDialog::getExistingDirectory(this,"选择路径，将解密路径下所有abt文件");
    QFileInfoList files;
    getAllAbt(path,files);
    if(files.isEmpty())return;
    for(int i=0;i<files.size();i++){
        QFile(files.at(i).absoluteFilePath()).rename(files.at(i).absolutePath()+"/"+filenameDecrypt(files.at(i).absoluteFilePath()));
    }
    QMessageBox::information(this,"消息","文件解密完成!","确认");
}


void MainWindow::on_btn_deleteBkedlist_clicked()
{
    if(!isProcessing&&!isAutoDeleting){
        int ret = QMessageBox::question(this,"删除提示","删除已备份文件列表将可能造成重复备份,您确定要删除吗？","确定","取消");
        if(ret == 0){
            ret = QMessageBox::question(this,"删除提示","删除已备份文件列表将可能造成重复备份,您真的确定要删除吗？","确定","取消");
            if(ret == 0){
                ret = QMessageBox::question(this,"删除提示","您确定真的没有手滑吗？","确定","我手滑了");
                if(ret == 0){
                    deleteBkedlistFile();
                    QMessageBox::information(this,"消息","操作完成。","确定");
                }
            }
        }
    }else{
        QMessageBox::information(this,"消息","当前有任务正在进行，请稍后再进行操作。","确定");
    }
}

void MainWindow::saveSource2TargetToFile(){
    if(ui->TableFilePath->rowCount()<=0)return;
    QFile fout(SOURCE2TARGET);
    fout.open(QFile::WriteOnly|QFile::Text);
    if(!fout.isOpen())return;
    fout.write((QString::number(ui->TableFilePath->rowCount())+"\n").toUtf8());
    for(int i=0;i<ui->TableFilePath->rowCount();i++){
        fout.write((ui->TableFilePath->item(i,0)->text()+"|"+ui->TableFilePath->item(i,1)->text()+"\n").toUtf8());
    }
    fout.close();
}

void MainWindow::initSource2TargetFromFile(){
    if(!QFileInfo::exists(SOURCE2TARGET))return;
    for(int i=ui->table_unbackup_list->rowCount()-1;i>=0;i--){
        ui->table_unbackup_list->removeRow(i);
    }
    QFile fin(SOURCE2TARGET);
    fin.open(QFile::ReadOnly|QFile::Text);
    if(!fin.isOpen())return;
    int num=QString::fromUtf8(fin.readLine()).toInt();
    for(int i=0;i<num;i++){
        QString data;
        QStringList s;
        while(s.size()<2)
        {
            data=QString::fromUtf8(fin.readLine());
            data.remove('\n');
            s=data.split('|');
        }
        int rc=ui->TableFilePath->rowCount();
        ui->TableFilePath->insertRow(rc);
        QTableWidgetItem* item0 = new QTableWidgetItem(s[0]);
        ui->TableFilePath->setItem(rc,0,item0);
        QTableWidgetItem* item1 = new QTableWidgetItem(s[1]);
        ui->TableFilePath->setItem(rc,1,item1);
    }
    fin.close();
}

void MainWindow::initConfigByStr(QString str){
    if(str[0]=='0'){
        btn_enable_filter=false;
        ui->btn_enableFilter->setText("已关闭");
        ui->btn_filterByTime->setEnabled(false);
        ui->btn_filterByBdyun->setEnabled(false);
    }else{
        btn_enable_filter=true;
        ui->btn_enableFilter->setText("已开启");
        ui->btn_filterByTime->setEnabled(true);
        ui->btn_filterByBdyun->setEnabled(true);
    }
    if(str[1]=='0'){
        btn_filterByTime=false;
        ui->btn_filterByTime->setText("已关闭");
    }else{
        btn_filterByTime=true;
        ui->btn_filterByTime->setText("已开启");
    }
    if(str[2]=='1'){
        needClickAutorun=true;
    }
    if(str[3]=='1'){
        btn_filterByBdyun=true;
        ui->btn_filterByBdyun->setText("已开启");
    }else{
        btn_filterByBdyun=false;
        ui->btn_filterByBdyun->setText("已关闭");
    }
}
void MainWindow::saveConfigToFile(){
    if(!configInited)return;
    QFile fout(CONFIG_FILE),fout1(PASSWORD_FILE);
    fout.open(QFile::WriteOnly|QFile::Text);
    fout1.open(QFile::WriteOnly|QFile::Text);
    if(!fout.isOpen())return;
    QString str;
    if(btn_enable_filter==false)str+="0";
    else str+="1";
    if(btn_filterByTime==false)str+="0";
    else str+="1";
    if(autorun==false)str+="0";
    else str+="1";
    if(btn_filterByBdyun==false)str+="0";
    else str+="1";
    fout.write((str+"|"+*Target_Path+"|"+QString::number(Max_Filesize)+"|"+*bz_path+"|"+*bdyunpath).toUtf8());
    fout.close();
    if(!fout1.isOpen())return;
//    qDebug()<<*password;
    fout1.write(password->toUtf8());
    fout1.close();
}

bool MainWindow::ConfigInitialFromFile(){
    QFile fin(CONFIG_FILE),fin1(PASSWORD_FILE);
    fin.open(QFile::ReadOnly|QFile::Text);
    fin1.open(QFile::ReadOnly|QFile::Text);
    if(!fin.isOpen())return false;
    QStringList con = QString::fromUtf8(fin.readAll()).split("|");
    initConfigByStr(con[0]);
    *Target_Path=con[1];
    Max_Filesize=QString(con[2]).toInt();
    *bz_path=con[3];
    if(con.size()>4){
        *bdyunpath=con[4];
    }else *bdyunpath="";
    if(fin1.isOpen()){
        *password=QString::fromUtf8(fin1.readAll());
//        qDebug()<<*password;
    }
    return true;
}

void MainWindow::saveThreadsNumToFile(){
    if(cpuThreads<0)return;
    QFile fout(THREADS);
    fout.open(QFile::WriteOnly|QFile::Text);
    if(!fout.isOpen())return;
    fout.write(QString::number(cmdThreads).toUtf8());
    fout.close();
}
void MainWindow::initThreadsNumFromFile(){
    if(cpuThreads<0)return;
    QFile fin(THREADS);
    fin.open(QFile::ReadOnly|QFile::Text);
    if(!fin.isOpen())return;
    bool ok=false;
    int num=QString::fromUtf8(fin.readAll()).toInt(&ok);
    if(ok&&num>0&&num<=cpuThreads){
        ui->slider_threads->setValue(num);
    }
    fin.close();
}

void MainWindow::on_btn_autorun_clicked()
{
    if(!autorun){
        ui->btn_autorun->setText("已开启");
        if(!btn_enable_filter){
            emit ui->btn_enableFilter->clicked();
        }
        ui->btn_autoDelete->setEnabled(false);
        ui->btn_deleteBkedlist->setEnabled(false);
        ui->btn_enableFilter->setEnabled(false);
        ui->btn_filterByTime->setEnabled(false);
        ui->btn_filterByBdyun->setEnabled(false);
        ui->Btn_startBackup->setEnabled(false);
        ui->BtnAddFilePath->setEnabled(false);
        ui->BtnDeleteFilePath->setEnabled(false);
        ui->btn_stopBackup->setEnabled(false);
        
        autorun=true;
        emit ui->btn_autoDelete->clicked();
        timer_autorun->start(2*60*1000);
    }else{
        ui->btn_autorun->setText("已关闭");
        
        if(!isAutoDeleting)ui->btn_autoDelete->setEnabled(true);
        ui->btn_deleteBkedlist->setEnabled(true);
        ui->btn_enableFilter->setEnabled(true);
        ui->btn_filterByTime->setEnabled(true);
        ui->btn_filterByBdyun->setEnabled(true);
        if(!isProcessing)ui->Btn_startBackup->setEnabled(true);
        ui->BtnAddFilePath->setEnabled(true);
        ui->BtnDeleteFilePath->setEnabled(true);
        ui->btn_stopBackup->setEnabled(true);
        
        autorun=false;
        timer_autorun->stop();
    }
}

void MainWindow::slot_timer_timeout(){
    emit ui->btn_autoDelete->clicked();
}


quint64 MainWindow::getDirSize(const QString& filePath){

        QDir tmpDir(filePath);
        quint64 size = 0;
        if(QFileInfo(filePath).isFile())return QFileInfo(filePath).size();
        /*获取文件列表  统计文件大小*/
        foreach(QFileInfo fileInfo, tmpDir.entryInfoList(QDir::Files))
        {
            size += fileInfo.size();
        }

        /*获取文件夹  并且过滤掉.和..文件夹 统计各个文件夹的文件大小 */
        foreach(QString subDir, tmpDir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
        {
            size += getDirSize(filePath + QDir::separator() + subDir); //递归进行  统计所有子目录
        }

        return size;

}



void MainWindow::on_btn_stopBackup_clicked()
{
    if(!isProcessing){
        mark_stopBtn=false;
        QMessageBox::information(this,"消息","当前没有备份进程正在运行中。","确认");
        return ;
    }

    if(!mark_stopBtn)mark_stopBtn=true;
    ui->btn_stopBackup->setEnabled(false);
}

void MainWindow::on_btn_filterByBdyun_clicked()
{
    if(btn_filterByBdyun){
        btn_filterByBdyun=false;
        ui->btn_filterByBdyun->setText("已关闭");
    }else{
        btn_filterByBdyun=true;
        ui->btn_filterByBdyun->setText("已开启");
    }
}

void MainWindow::on_slider_threads_valueChanged(int value)
{
    if(cpuThreads<0)return;
    cmdThreads=value;
    ui->label_num_threads->setText(QString::number(value));
}
