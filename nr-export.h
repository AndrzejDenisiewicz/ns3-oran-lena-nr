// SPDX-License-Identifier: GPL-2.0-only

#ifndef NR_EXPORT_H
#define NR_EXPORT_H

#if defined(_WIN32) && defined(nr_EXPORTS)
#define NR_EXPORT __declspec(dllexport)
#elif defined(_WIN32)
#define NR_EXPORT __declspec(dllimport)
#else
#define NR_EXPORT
#endif

#endif // NR_EXPORT_H