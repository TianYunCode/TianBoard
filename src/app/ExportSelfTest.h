/**
 * @file ExportSelfTest.h
 * @brief 导出链路端到端自检的声明。
 */

#pragma once

/// @brief PDF 与 .tbz 导出的产出物自检入口。
namespace ExportSelfTest
{
    /**
     * @brief 检查三个导出器产出的文件真是"能读回来、页数对、内容对"的东西。
     *
     * 详细说明：`persistsDocument()` 是导出器唯一的非交互入口，界面上点"导出"只看得到
     * "没报错"。这条自检把两页各有不同内容的文档真导一遍，再用 Qt 的 PDF 读回器与 .tbz
     * 导入器逐项对账：页数、每页尺寸（点）、每页文字属于哪一页、渲染出来有没有墨、
     * 导出结束后场景的深浅底有没有恢复且文档页数没变、打包再导入之后每一页的内容指纹是否一致，
     * 最后拿同一个包答"同名替换"再导入一次——那条路要走过 `removeRows`，正是它撞出了
     * `DocumentTreeModel::mCurrentNode` 未初始化这个崩溃。
     * 临时文档与产物自建自删，用户数据不碰。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace ExportSelfTest
