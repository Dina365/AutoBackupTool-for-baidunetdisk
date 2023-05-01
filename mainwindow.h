#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "config.h"
#include "itemeditdialog.h"
#include "unbackupfilelist.h"
#include "filelist.h"
#include <QMainWindow>
#include <QFileDialog>
#include <QFileInfo>
#include <QFileInfoList>
#include <QDebug>
#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QMessageBox>
#include <QMouseEvent>
#include <QVector>
#include <QTableWidget>
#include <QDir>
#include <QProcess>
#include <QFile>
#include <QMetaType>
#include <QTimer>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void closeEvent(QCloseEvent *event);
private slots:
    void on_BtnAddFilePath_clicked();

    void on_BtnDeleteFilePath_clicked();


    void on_TableFilePath_cellDoubleClicked(int row, int column);


//    void on_TableFilePath_itemClicked(QTableWidgetItem *item);

    void on_btn_select_path_clicked();
    
    void on_textEdit_targetPath_textChanged();

    void on_textEdit_password_textChanged();

    void on_textEdit_MaxFileSize_textChanged();

    void on_textEdit_bzpath_textChanged();

    void on_Btn_ensure_settings_clicked();

    void on_btn_select_path_bz_clicked();

    void on_Btn_startBackup_clicked();

    void slotCmdFinished(int exitCode, QProcess::ExitStatus exitStatus);

    void on_btn_select_path_bdyun_clicked();

    void on_textEdit_bdyunpath_textChanged();

    void on_btn_enableFilter_clicked();

    void on_btn_filterByTime_clicked();

    void on_startAutoDelete();

    void on_finishedAutoDelete(bool code);

    void on_btn_autoDelete_clicked();

    void on_btn_filenameDecrypt_clicked();

    void on_btn_deleteBkedlist_clicked();

    void on_btn_autorun_clicked();
    
    void slot_timer_timeout();

    void on_btn_stopBackup_clicked();

    void on_btn_filterByBdyun_clicked();

    void on_slider_threads_valueChanged(int value);

private:
    Ui::MainWindow *ui;
    
    QString* Target_Path;
    QString* bz_path;
    QString* bdyunpath;
    int Max_Filesize;//最大备份文件阈值
    QString* password;//解压密码
    QProcess *cmd;//压缩进程
    int cpuThreads;//cpu逻辑线程数
    int cmdThreads;//压缩线程数
    QList<File2target>::iterator target;//备份目标
    int error_num=0;//备份发生错误数
    UnbackupFileList* unbkFilelist;//未备份文件列表
    FileList* bkedlist;//已备份文件列表
    FileList* updatedlist;//备份文件更新时会添加到此列表
    void getAllAbt(const QString& path,QFileInfoList& list);
    quint64 getDirSize(const QString &filePath);
    int getLocalFileSize();//得到备份目标目录的总文件大小
    int getUnbkedFileSize();//得到未备份的文件总大小
    void initBkedlistFromFile();//从文件中读入已备份文件列表
    void saveBkedlistToFile();//将已备份文件列表保存到文件
    void initUpdlistFromFile();//从文件读入备份文件更新后重备份的名单
    void saveUpdlistToFile();//将备份文件更新后重备份的名单存入文件
    void initSource2TargetFromFile();//从文件中读取已设置的源路径和目标
    void saveSource2TargetToFile();//将已设置的源路径和目标保存到文件
    void deleteBkedlistFile();//删除已备份列表的记录文件
    void filter();//从未备份列表中过滤已备份列表
    //FileList* allfilelist;//从目录提取的文件名单
    bool Btn_ensure_enable=true;
    bool isProcessing=false;//标识正在进行备份处理
    bool isAutoDeleting=false;//标识正在进行自动删除
    bool mark_stopBtn=false;//停止按钮触发
    bool btn_enable_filter;//过滤器开关
    bool btn_filterByTime;//设置过滤器是否检索文件修改时间。开启后更新的文件将不再被过滤
    bool btn_filterByBdyun;//从百度网盘已上传文件中筛除
    bool autorun=false;//自动备份开关
    bool needClickAutorun=false;
    QTimer* timer_autorun;
    bool chkcfg();//检查设置是否合法，不合法则屏蔽按钮
    
//    int selected_row_TableFilePath = 0;//记录TableFilePath中被选中的行
/*
 待实现：

*///>>>>>>>>>>>>>>>>>>>>>>>>待实现
    void ConfigInitial();
    void initConfigByStr(QString str);
    bool ConfigInitialFromFile();
    void saveConfigToFile();
    bool configInited=false;

    void saveThreadsNumToFile();
    void initThreadsNumFromFile();

    void getUnbackupFilelist();
    void bz_process(const File2target& file_target);
    void startCmd();//运行压缩进程
    QString filenameEncrypt(const QString &filename);//文件名加密。方法是将一个unicode字符的每四位拆分成一个字节，并且加上‘A’。
    QString filenameDecrypt(const QString &filename);//文件名解密
protected:
    //void mousePressEvent(QMouseEvent* event);
signals:
    void startAutoDelete();
    void finishedAutoDelete(bool code);
};

#endif // MAINWINDOW_H
