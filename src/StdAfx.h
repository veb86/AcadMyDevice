// StdAfx.h — общие заголовки ObjectARX 2021 для проекта MyDevice.
#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(_DEBUG) && !defined(AC_FULL_DEBUG)
#error _DEBUG should not be defined except in internal Adesk debug builds
#endif

#include <windows.h>
#include <tchar.h>

#include "rxregsvc.h"
#include "accmd.h"
#include "aced.h"
#include "adslib.h"
#include "acutads.h"
#include "dbmain.h"
#include "dbsymtb.h"
#include "dbents.h"
#include "dbdynblk.h"
#include "dbsymutl.h"
#include "dbapserv.h"
#include "dbfiler.h"
#include "dbproxy.h"
#include "acgi.h"
#include "acgiutil.h"
#include "gepnt3d.h"
#include "gevec3d.h"
#include "gemat3d.h"
#include "geassign.h"
#include "AcString.h"
#include "acestext.h"
