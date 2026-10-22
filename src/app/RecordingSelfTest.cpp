#include "app/RecordingSelfTest.h"

#include "core/Application.h"
#include "recording/RecordingController.h"
#include "settings/Settings.h"

#include <QDeadlineTimer>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QAudioDevice>
#include <QMediaDevices>
#include <QMediaPlayer>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVector>

namespace
{
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

        const QString     recordingDirectory = Settings::userRecordingDirectory();
        const QStringList before             = QDir(recordingDirectory).entryList(QDir::Files);

        qInfo() << "recording-selftest: 录制目录" << recordingDirectory;
        qInfo() << "recording-selftest: 可用麦克风数量" << QMediaDevices::audioInputs().size();

        // 控制器是懒建单例（构造函数是 private 的）：界面那条路也是走 instance()。
        RecordingController *controller = RecordingController::instance();

        QVector<RecordingController::RecordingState> transitions;
        QObject::connect(controller, &RecordingController::recordingStateChanged,
            [&transitions](RecordingController::RecordingState state)
            {
                transitions.append(state);
            });

        // ---- 1. 启动：抓屏与编码器必须真的开起来 ----
        controller->start();
        if (controller->recordingState() != RecordingController::Recording)
        {
            failures << QStringLiteral("start() 之后状态不是 Recording（实际 %1），录屏根本没起来").arg(controller->recordingState());
            return failures.count();
        }

        // 音频只是诊断：没麦克风时退化成一趟"只录画面"，那是设计好的路径。
        qInfo() << "recording-selftest: 本段是否在采音频" << controller->isCapturingAudio();

        // ---- 2. 采两秒：让抓帧定时器与音频队列有机会喂写入器 ----
        pump(2000);

        controller->stop();

        QDeadlineTimer finalizeDeadline(15000);
        if (!waitFor(finalizeDeadline, 100,
                [&controller]
                {
                    return controller->recordingState() == RecordingController::Stopped;
                }))
        {
            failures << QStringLiteral("stop() 之后 15 秒还没回到 Stopped：编码器收尾没走完");
        }

        // ---- 3. 状态序列 ----
        if (!transitions.contains(RecordingController::Recording) || !transitions.contains(RecordingController::Stopped))
        {
            failures << QStringLiteral("recordingStateChanged 没把 起→停 都报出来（收到 %1 次）").arg(transitions.size());
        }

        // ---- 4. 盘上必须多出一个 .wmv ----
        const QStringList after = QDir(recordingDirectory).entryList(QDir::Files);
        QStringList       fresh;
        for (const QString &name : after)
        {
            if (!before.contains(name))
            {
                fresh << name;
            }
        }

        if (fresh.isEmpty())
        {
            failures << QStringLiteral("录制目录里没出现新文件（%1 个文件，无新增）").arg(after.size());
            return failures.count();
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

        // ---- 收尾：别把测试产物留在用户机器上 ----
        // 播放器必须已经析构：它背后的 ffmpeg 还握着文件句柄时，Windows 上删不掉，
        // 而这会被记成一条自检失败（其实录制本身是好的）。
        player.setSource(QUrl());
        pump(150);

        if (!QFile::remove(filePath))
        {
            pump(500);
            if (!QFile::remove(filePath))
            {
                failures << QStringLiteral("自检没能删掉自己的产物: %1").arg(filePath);
            }
        }

        for (const QString &failure : failures)
        {
            qWarning() << "recording-selftest:" << failure;
        }

        return failures.count();
    }
} // namespace RecordingSelfTest
