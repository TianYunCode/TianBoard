#include "app/RecordingSelfTest.h"

#include "recording/RecordingController.h"
#include "settings/Settings.h"

#include <QAudioDevice>
#include <QDeadlineTimer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QMetaObject>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>

namespace
{
    /**
     * @brief 状态序列记在进程级静态量上，不记在 run() 的局部变量里。
     * @note RecordingController 是单例，活得比这个函数久。收尾超时那条路上录制还在跑，
     * 函数返回后它再发一次 recordingStateChanged，按引用捕获局部容器就是往已析构的
     * QVector 里写。下面照样会显式断开连接，静态量是第二道保险。
     */
    QVector<RecordingController::RecordingState> gTransitions;

    /// @brief 让事件循环转 ms 毫秒：抓屏定时器、跨线程的音频队列与收尾都要靠这段时间跑。
    void pump(int ms)
    {
        QEventLoop loop;
        QTimer::singleShot(ms, &loop, &QEventLoop::quit);
        loop.exec();
    }

    /// @brief 反复泵事件直到 pred 成立或超过 deadline；返回是否成立。
    template <typename Predicate> bool waitFor(const QDeadlineTimer &deadline, int stepMs, Predicate pred)
    {
        while (!pred() && !deadline.hasExpired())
        {
            pump(stepMs);
        }

        return pred();
    }

    /**
     * @brief ASF（.wmv/.asf）容器开头的四个字节。
     * @note Start Header 的 GUID 是 {75B22630-668E-11CF-A6D9-00AA0062CE6C}，前四字节按小端
     * 存放成 30 26 B2 75——按字面抄成 30 26 A6 75 是错的（实测真录出来的 .wmv 就报在这里）。
     */
    const char kAsfStartHeaderGuidPrefix[4] = {static_cast<char>(0x30), static_cast<char>(0x26), static_cast<char>(0xB2), static_cast<char>(0x75)};
} // namespace

namespace RecordingSelfTest
{
    int run()
    {
        QStringList failures;

        const QString recordingDirectory = Settings::userRecordingDirectory();

        qInfo() << "recording-selftest: 录制目录" << recordingDirectory;
        qInfo() << "recording-selftest: 可用麦克风数量" << QMediaDevices::audioInputs().size();

        /**
         * @brief 每个出口都走这里：先把每条不一致打到日志，再返回条数。
         * @note 少了打印这一步，提前 return 的那几条失败就丢了——外层用例只会说
         * "录屏链路有 1 处不一致"加上一整屏正常启动噪音，把人往错的方向带。
         */
        auto finish = [&failures]()
        {
            for (const QString &failure : failures)
            {
                qWarning() << "recording-selftest:" << failure;
            }

            return failures.count();
        };

        // 先记下目录现状；名字里带 Recording- 的残留只报不改，别人的文件不由自检来删。
        const QStringList before = QDir(recordingDirectory).entryList(QDir::Files);
        for (const QString &stale : before)
        {
            if (stale.startsWith(QStringLiteral("Recording-")))
            {
                qWarning() << "recording-selftest: 录制目录里留有上一次的" << stale << "（本自检不删别人的文件）";
            }
        }

        // 控制器是懒建单例（构造函数是 private 的）：界面那条路也是走 instance()。
        RecordingController *controller = RecordingController::instance();

        gTransitions.clear();
        const QMetaObject::Connection watcher = QObject::connect(controller, &RecordingController::recordingStateChanged,
            [](RecordingController::RecordingState state)
            {
                gTransitions.append(state);
            });

        // ---- 1. 启动：抓屏与编码器必须真的开起来 ----
        controller->start();
        if (controller->recordingState() != RecordingController::Recording)
        {
            failures << QStringLiteral("start() 之后状态不是 Recording（实际 %1），录屏根本没起来").arg(controller->recordingState());
            QObject::disconnect(watcher);
            return finish();
        }

        // 音频只是诊断：没麦克风时退化成一趟"只录画面"，那是设计好的路径。
        qInfo() << "recording-selftest: 本段是否在采音频" << controller->isCapturingAudio();

        // ---- 2. 采两秒：让抓帧定时器与音频队列有机会喂写入器 ----
        pump(2000);

        controller->stop();

        QDeadlineTimer finalizeDeadline(15000);
        if (!waitFor(finalizeDeadline, 100,
                [controller]
                {
                    return controller->recordingState() == RecordingController::Stopped;
                }))
        {
            failures << QStringLiteral("stop() 之后 15 秒还没回到 Stopped：编码器收尾没走完");
        }

        // 观察要到这儿才撤：再早一次就把收尾那次 Stopped 从序列里抹掉了。
        QObject::disconnect(watcher);

        // ---- 3. 状态序列 ----
        if (!gTransitions.contains(RecordingController::Recording) || !gTransitions.contains(RecordingController::Stopped))
        {
            failures << QStringLiteral("recordingStateChanged 没把 起→停 都报出来（收到 %1 次）").arg(gTransitions.size());
        }

        // ---- 4. 盘上必须多出一个 .wmv ----
        QStringList fresh;
        for (const QString &name : QDir(recordingDirectory).entryList(QDir::Files))
        {
            if (!before.contains(name))
            {
                fresh << name;
            }
        }

        if (fresh.isEmpty())
        {
            failures << QStringLiteral("录制目录里没出现新文件");
            return finish();
        }
        if (fresh.size() > 1)
        {
            failures << QStringLiteral("一趟录制产出 %1 个新文件: %2").arg(fresh.size()).arg(fresh.join(", "));
        }

        const QString   filePath = QDir(recordingDirectory).filePath(fresh.first());
        const QFileInfo info(filePath);
        const qint64    size = info.size();

        if (!info.fileName().startsWith(QStringLiteral("Recording-")))
        {
            failures << QStringLiteral("录制文件名不合约定: %1").arg(info.fileName());
        }
        if (!info.fileName().endsWith(QStringLiteral(".wmv"), Qt::CaseInsensitive))
        {
            failures << QStringLiteral("录制文件扩展名不是 .wmv: %1").arg(info.fileName());
        }
        if (size < 2048)
        {
            failures << QStringLiteral("录制文件只有 %1 字节，像是没写入样本的空壳").arg(size);
        }

        // ---- 5. 容器头：写完收尾才会落下 ASF 的 Start Header ----
        QFile file(filePath);
        if (!file.open(QIODevice::ReadOnly))
        {
            failures << QStringLiteral("录制文件打不开: %1").arg(file.errorString());
        }
        else
        {
            const QByteArray header = file.read(4);
            file.close();

            if (header.size() != 4 || memcmp(header.constData(), kAsfStartHeaderGuidPrefix, 4) != 0)
            {
                failures << QStringLiteral("前 4 个字节不是 ASF Start Header 的 GUID 前缀（读到 %1），"
                                           "说明写入器没正常收尾")
                                .arg(QString::fromLatin1(header.toHex(' ')));
            }
        }

        // ---- 6. 交给多媒体后端读一遍：能读出正数时长才算真录出来了 ----
        // 这个作用域必须在删文件之前结束：播放器背后的 ffmpeg 还握着文件句柄时 Windows 删不掉，
        // 而那会被记成一条自检失败——其实录制本身是好的。
        {
            QMediaPlayer player;

            QMediaPlayer::MediaStatus status = QMediaPlayer::NoMedia;
            QObject::connect(&player, &QMediaPlayer::mediaStatusChanged,
                [&status](QMediaPlayer::MediaStatus s)
                {
                    status = s;
                });

            player.setSource(QUrl::fromLocalFile(filePath));

            QDeadlineTimer probeDeadline(15000);
            const bool     loaded = waitFor(probeDeadline, 100,
                [&status]
                {
                    return status == QMediaPlayer::LoadedMedia || status == QMediaPlayer::InvalidMedia;
                });

            if (!loaded)
            {
                failures << QStringLiteral("15 秒内多媒体后端没给出结论（status=%1）").arg(status);
            }
            else if (status == QMediaPlayer::InvalidMedia)
            {
                failures << QStringLiteral("录出来的文件被多媒体后端判为无效: %1").arg(player.errorString());
            }
            else
            {
                const qint64 duration = player.duration();
                qInfo() << "recording-selftest: 时长" << duration << "ms，文件大小" << size << "字节";

                if (duration < 500)
                {
                    failures << QStringLiteral("时长只有 %1 ms（录了约 2 秒），说明帧没真进容器").arg(duration);
                }
            }

            player.stop();
        }

        // ---- 收尾：这一趟产生的文件全部删掉 ----
        pump(150); // 给后端一点时间把句柄交还
        for (const QString &name : fresh)
        {
            const QString path = QDir(recordingDirectory).filePath(name);

            if (!QFile::remove(path))
            {
                pump(500);
                if (!QFile::remove(path))
                {
                    failures << QStringLiteral("自检没能删掉自己的产物: %1").arg(path);
                }
            }
        }

        return finish();
    }
} // namespace RecordingSelfTest
