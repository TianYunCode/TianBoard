/**
 * @file TestProcessSandbox.h
 * @brief 给被拉起的 TianBoard.exe 换一个可写根目录：自检与冒烟不该往用户的文档库里写东西。
 *
 * 详细说明：应用把文档仓库、日志、缓存都放在 Settings::userDataDirectory() 下，那里默认是
 * %APPDATA%/TianBoard。而自检与冒烟跑的是真应用——启动就会新建一份文档并持久化，一次 ctest
 * 就是七份。这里给每个测试进程一个自己的临时根目录（经 TIANBOARD_DATA_DIR 环境变量传下去），
 * 析构时整目录删掉；代价是自检的日志落在那个目录的 log/ 里而不是 %APPDATA%。
 */

#pragma once

#include <QCoreApplication>
#include <QDir>
#include <QProcess>
#include <QProcessEnvironment>

/// @brief 一次性沙箱：构造时把目录写进子进程环境，析构时删掉整个目录。
class TestProcessSandbox
{
public:
    /// @param process 即将启动应用的那个 QProcess（环境必须在 start() 之前设）。
    /// @param tag 目录名里的用例标识，方便失败时手工去看现场。
    TestProcessSandbox(QProcess &process, const QString &tag)
        : mRoot(QDir(QDir::tempPath()).filePath(QStringLiteral("TianBoard-%1-%2").arg(tag).arg(QCoreApplication::applicationPid())))
    {
        QDir(mRoot).removeRecursively();

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("TIANBOARD_DATA_DIR"), mRoot);
        process.setProcessEnvironment(env);
    }

    ~TestProcessSandbox()
    {
        QDir(mRoot).removeRecursively();
    }

    /// 沙箱根目录，失败信息里带出来供手工复查。
    QString root() const
    {
        return mRoot;
    }

private:
    QString mRoot;
};
