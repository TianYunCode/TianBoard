/**
 * @file MediaInsertSelfTest.h
 * @brief 音视频插入这条链路的端到端自检声明。
 */

#pragma once

/// @brief 把本地音频插进画板、存盘、再冷读回来的自检入口。
namespace MediaInsertSelfTest
{
    /**
     * @brief 验"拖进来的本地媒体到底有没有进文档"。
     *
     * 详细说明：`BoardController::downloadFinished()` 的音频/视频分支在拿到字节时会把媒体
     * 复制进文档目录，但没拿到字节（本地文件走的就是这条）时直接沿用外部源文件的 URL；
     * 而页写出器永远按 `audios/<文件名>`、`videos/<文件名>` 这种文档内相对路径写引用。
     * 两者一叠加，表现就是"把本地音频拖进画板 → 保存 → 重开，音频没了"，
     * 与今天修掉的内嵌 SVG 引用悬空是同一类缺陷。
     * 自检现场合成一个合法的小 WAV 放在文档目录之外，分别走"没有字节"与"带字节"两条分支，
     * 要求两条都把文件落进文档的 audios/ 里、页引用指得动、冷读回来音频图元还在。
     * 临时文档与临时媒体文件自建自删，用户数据不碰。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace MediaInsertSelfTest
