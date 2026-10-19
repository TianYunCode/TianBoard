#pragma once

#include <QtCore/qglobal.h>

#if defined(TB_IWB_LIBRARY)
#define TB_IWB_EXPORT Q_DECL_EXPORT
#else
#define TB_IWB_EXPORT Q_DECL_IMPORT
#endif
