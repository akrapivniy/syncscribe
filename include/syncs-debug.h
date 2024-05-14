/**************************************************************
 * Description: SyncScribe library to manage network and local events,
 * variables and channels
 * Copyright (c) 2022 Alexander Krapivniy (a.krapivniy@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 ***************************************************************/
#ifndef SYNCS_DEBUG_H
#define	SYNCS_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>


#define SYNCSD_STREAM stdout

#ifdef SYNCSD_STREAM
#define MSG(fmt,args...)	fprintf (SYNCSD_STREAM, MODULE_NAME":" fmt "\n", ##args)
#define ERROR(fmt,args...)	fprintf (SYNCSD_STREAM, MODULE_NAME" error:%s:" fmt "\n",__func__, ##args)
#define DEBUG(fmt,args...)	fprintf (SYNCSD_STREAM, MODULE_NAME":%s:%d:" fmt "\n",__func__, __LINE__, ##args)
#define DUMP_VAL(name)          fprintf (SYNCSD_STREAM, #name"[%08x] = 0x%08x",&name,name)
#define DUMP_RES32(name)        fprintf (SYNCSD_STREAM, #name" = 0x%08x",name)
#define DUMP_RES8(name)         fprintf (SYNCSD_STREAM, #name" = 0x%02x",(uint32_t)name)
#else
#define MSG(fmt,args...)
#define ERROR(fmt,args...)
#define DEBUG(fmt,args...)
#define DUMP_VAL(name)
#define DUMP_RES32(name)
#define DUMP_RES8(name)
#endif

#define syncsd_info		MSG
#define syncsd_debug		DEBUG
#define syncsd_error		ERROR

#ifdef __cplusplus
}
#endif

#endif	/* SYNCS_DEBUG_H */

