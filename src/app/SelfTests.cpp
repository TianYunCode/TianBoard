#include "app/SelfTests.h"

#include <QLatin1String>
#include <QTimer>

#include "app/AutoSaveSelfTest.h"
#include "app/ClipboardSelfTest.h"
#include "app/CookieSelfTest.h"
#include "app/DownloadSelfTest.h"
#include "app/EraserSelfTest.h"
#include "app/ExportSelfTest.h"
#include "app/FormatSelfTest.h"
#include "app/ImportSelfTest.h"
#include "app/MediaInsertSelfTest.h"
#include "app/PageOperationsSelfTest.h"
#include "app/PreferenceLiveApplySelfTest.h"
#include "app/RecordingSelfTest.h"
#include "app/ShortcutSelfTest.h"
#include "app/SnapSelfTest.h"
#include "app/UndoRedoSelfTest.h"
#include "core/Application.h"

void installSelfTestFromArgs(Application &app)
{
    struct SelfTestMode
    {
        const char *flag;
        int (*run)();
    };

    static const SelfTestMode selfTestModes[] = {
        {"--format-selftest",    FormatSelfTest::run             }, // 文档格式读写往返（tst_format）
        {"--snap-selftest",      SnapSelfTest::run               }, // 网格吸线的场景侧接线（tst_snap）
        {"--shortcut-selftest",  ShortcutSelfTest::run           }, // 快捷键注册表的读写与冲突（tst_shortcuts）
        {"--cookie-selftest",    CookieSelfTest::run             }, // Cookie 镜像罐与退出清理计数（tst_cookies）
        {"--import-selftest",    ImportSelfTest::run             }, // 导入重名裁决三条路径 + 导入产物对账（tst_import）
        {"--recording-selftest", RecordingSelfTest::run          }, // 录屏端到端：容器、时长与可播（tst_recording）
        {"--clipboard-selftest", ClipboardSelfTest::run          }, // 复制→粘贴→删原件→存读往返（tst_clipboard）
        {"--export-selftest",    ExportSelfTest::run             }, // PDF 页数/尺寸/内容与 .tbz 往返（tst_export）
        {"--undoredo-selftest",  UndoRedoSelfTest::run           }, // 撤销/重做全回路与换文档清栈（tst_undoredo）
        {"--autosave-selftest",  AutoSaveSelfTest::run           }, // 自动保存定时器起停与真落盘（tst_autosave）
        {"--pages-selftest",     PageOperationsSelfTest::run     }, // 页面增删复制移动后逐页冷读对账（tst_pages）
        {"--media-selftest",     MediaInsertSelfTest::run        }, // 音视频插入有没有变成文档的一部分（tst_media）
        {"--download-selftest",  DownloadSelfTest::run           }, // 本地下载端到端与跨层事实的应答连线（tst_download）
        {"--preflive-selftest",  PreferenceLiveApplySelfTest::run}, // 偏好设置改完当场生效，不重启不换页（tst_preflive）
        {"--eraser-selftest",    EraserSelfTest::run             }, // 橡皮切开一段笔迹的整条回路与存读对账（tst_eraser）
    };

    const QStringList args = app.arguments();

    for (const SelfTestMode &mode : selfTestModes)
    {
        if (!args.contains(QLatin1String(mode.flag)))
        {
            continue;
        }

        if (app.isRunning())
        {
            qWarning() << mode.flag << "：已有实例在运行，让路";
            app.setSelfTestRun(0,
                []
                {
                    return 75;
                });
            continue;
        }

        // 1.5 秒的延时是给装配留的：主窗口、各控制器与信号连线都在 exec() 里建，
        // 早于那一刻跑自检只会量到半装配状态。
        app.setSelfTestRun(1500, mode.run);
    }
}
