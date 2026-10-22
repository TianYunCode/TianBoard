/**
 * @file TrashSelfTest.h
 * @brief 文档回收站这条链路的端到端自检声明。
 */

#pragma once

/// @brief 移入回收站、清空回收站、按天数自动清理旧文档的自检入口。
namespace TrashSelfTest
{
    /**
     * @brief 把"删掉的文档到底去了哪、有没有真从盘上消失"量一遍。
     *
     * 详细说明：这条链路每一步都动盘上的目录，而且分组归属只写在各文档的 metadata.rdf 里
     * （`folders.xml` 只存目录树，不存文档），错了既不崩也不报错，只表现为
     * "回收站里清空之后磁盘还留着一包"或"重启之后删掉的文档又回到我的文档"。这里钉四件事：
     * ① 界面上那条移入回收站的路（`DocumentController::moveToTrash`）之后文档真的挂在回收站那一层，
     * 别的文档不受影响，且盘上的包还在（回收站不是删除）；② 新归属真的写进这份文档自己的
     * metadata.rdf——那才是下次启动重建树时读的东西；③ 界面上那条清空路
     * （`EmptyTrash` 在界面上映射成 `emptyFolder(index, CompleteDelete)`）把包从盘上删干净，
     * 树里的行也摘掉，而留在外面的文档一个都不许少；④ 退出前按天数自动清理只删过期的那一份。
     * 全程只动沙箱数据目录里的临时文档，用户真实的文档库不碰。
     *
     * @return 不一致的条数；0 表示通过。
     */
    int run();
} // namespace TrashSelfTest
