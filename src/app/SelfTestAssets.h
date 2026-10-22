/**
 * @file SelfTestAssets.h
 * @brief 自检共用的合成素材：给"用户从文档外面拖一个文件进来"这类用例一份字节确定的原料。
 *
 * 素材必须是**合法**的（导入器/播放器真能认），又要每次生成都一模一样，
 * 才能拿整段字节当指纹去比对落盘的产物。
 */

#pragma once

#include <QByteArray>
#include <QString>

/**
 * @brief 自检用的合成素材工厂。
 *
 * 线程约束：只在 GUI 线程的自检里调用，本身无状态。
 */
namespace SelfTestAssets
{
    /// @brief 一个合法的 8bit 单声道 WAV（8000Hz、0.1 秒）；同一进程内外每次调用字节都相同。
    QByteArray toneWav();

    /// @brief 把字节写进指定路径（覆盖写出）；写不出来时返回 false。
    bool writeBytes(const QString &path, const QByteArray &data);
} // namespace SelfTestAssets
