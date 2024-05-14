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

#include "syncs-types.h"


#ifdef SYNCS_SECURY_DATA

struct syncs_packet *syncs_crypt_encode_packet (uint8_t *key, struct syncs_packet *packet, uint8_t *crypt_buffer);
struct syncs_packet *syncs_crypt_decode_packet (uint8_t *key, struct syncs_packet *packet, uint8_t *crypt_buffer);

#else

#define syncs_crypt_encode_packet(a,b,c) (b)
#define syncs_crypt_decode_packet(a,b,c) (b)

#endif
