#pragma once
#ifndef WINRT_BASE_H
# include <Unknwn.h>
# include <winrt/base.h>
#else
# ifndef __IUnknown_INTERFACE_DEFINED__
#  error "<winrt/base.h> has been previously included without COM support"
# endif
#endif
