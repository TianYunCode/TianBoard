/**
 * @file RecordingSelfTest.h
 * @brief 录屏端到端自检的声明。
 */

#pragma once

/// @brief 录屏管线自检入口。
namespace RecordingSelfTest
{
    /**
     * @brief 真录一小段屏幕，验证容器、时长与可播性。
     *
     * 详细说明：录屏这条链路（抓屏定时器 → Windows Media 写入器 → waveIn 音频队列）过去只过
     * 编译与冒烟跑，从没产出过一个真文件。这里跑完整趟：start() 后让事件循环转约 2 秒（抓帧
     * 定时器与跨线程的音频队列都靠这段时间喂数据），stop() 收尾，然后逐项检查——目录里新增
     * 恰好一个 .wmv、文件不是空壳、头四个字节是 ASF 的 Start Header GUID 前缀（30 26 A6 75，
     * 容器写完才会落到位），最后交给 Qt 的多媒体后端读一遍，要求它报出正数时长。
     *
     * 音频是可选能力：没有麦克风时这条路会自己退化成"只录画面"，那是设计行为，所以这里只把
     * 有没有采到音当作诊断信息打印，不当失败条件；画面录不出来才是失败。
     *
     * 产物写在 `Settings::userRecordingDirectory()`（应用数据目录下的 recordings/），
     * 自检结束即删除，不留残档；tests/tst_recording 还会把整个数据根目录关进临时目录。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace RecordingSelfTest
