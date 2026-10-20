/**
 * @file IwbExport.h
 * @brief 定义 IWB 读写组件的动态库导入导出宏 TB_IWB_EXPORT。
 *
 * 构建 IWB 动态库时由 TB_IWB_LIBRARY 切换到 Q_DECL_EXPORT，其余目标走 Q_DECL_IMPORT。
 */

#pragma once

#include <QtCore/qglobal.h>

#if defined(TB_IWB_LIBRARY)
#define TB_IWB_EXPORT Q_DECL_EXPORT
#else
#define TB_IWB_EXPORT Q_DECL_IMPORT
#endif
