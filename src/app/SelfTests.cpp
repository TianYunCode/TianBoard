#include "app/SelfTests.h"

#include <QLatin1String>
#include <QTimer>

#include "app/CookieSelfTest.h"
#include "app/FormatSelfTest.h"
#include "app/ShortcutSelfTest.h"
#include "app/SnapSelfTest.h"
#include "core/Application.h"

void installSelfTestFromArgs(Application &app)
{
    struct SelfTestMode
    {
        const char *flag;
        int (*run)();
    };

    static const SelfTestMode selfTestModes[] = {
        {"--format-selftest",   FormatSelfTest::run  }, // 文档格式读写往返（tst_format）
        {"--snap-selftest",     SnapSelfTest::run    }, // 网格吸线的场景侧接线（tst_snap）
        {"--shortcut-selftest", ShortcutSelfTest::run}, // 快捷键注册表的读写与冲突（tst_shortcuts）
        {"--cookie-selftest",   CookieSelfTest::run  }, // Cookie 镜像罐与退出清理计数（tst_cookies）
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
