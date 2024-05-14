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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <openssl/evp.h>

#include "syncs-crypt.h"

#define CRC32_POLYNOMIAL 0xEDB88320

uint32_t crc32(uint8_t *data, uint16_t size)
{
	uint32_t crc = 0xFFFFFFFF;
	for (uint16_t i = 0; i < size; i++) {
		uint8_t byte = data[i];
		crc ^= byte;

		for (uint8_t j = 0; j < 8; j++) {
			if (crc & 1) {
				crc = (crc >> 1) ^ CRC32_POLYNOMIAL;
			} else {
				crc >>= 1;
			}
		}
	}
	return ~crc;
}

#define AES_BLOCK_SIZE 16
#define AES_KEY_SIZE 32

#ifdef SYNCS_SECURY_DATA
static uint8_t crypt_buffer[4096];

int calculate_aes256_cbc_ciphertext_length(int data_size)
{
    return ((data_size + (1 << 4) - 1) >> 4) << 4;
}

struct syncs_packet *syncs_crypt_encode_packet(uint8_t *key, struct syncs_packet *packet)
{
	uint32_t packet_data_size = SYNCS_PACKET_DATA_SIZE(packet->header.data_size);
	uint32_t size = SYNCS_CRYPT_HEADER_SIZE + packet_data_size;
	EVP_CIPHER_CTX *ctx;
	uint8_t *iv = key;
	uint8_t *encryption_key = key + AES_BLOCK_SIZE;
	uint32_t ciphertext_calc_size = calculate_aes256_cbc_ciphertext_length(size);
	uint32_t padding = ciphertext_calc_size - size;
	uint32_t encode_len;
	int len;

	packet->header.data_size = SYNCS_SET_PACKET_SIZE(ciphertext_calc_size - SYNCS_CRYPT_HEADER_SIZE, padding);

	packet->header.crc = crc32((uint8_t *)&packet->header.type, SYNCS_CRC_HEADER_SIZE + packet_data_size);

	memcpy(crypt_buffer, &packet->header.crc, SYNCS_CRYPT_HEADER_SIZE);
	memcpy(crypt_buffer + SYNCS_CRYPT_HEADER_SIZE, packet->buffer, packet_data_size);

	if (!(ctx = EVP_CIPHER_CTX_new()))
		return NULL;

	if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, encryption_key, iv) != 1)
		return NULL;

	if (EVP_EncryptUpdate(ctx, crypt_buffer, &len, crypt_buffer, size) != 1)
		return NULL;

	encode_len = len;

	if (EVP_EncryptFinal_ex(ctx, crypt_buffer + len, &len) != 1)
		return NULL;
	encode_len += len;
	EVP_CIPHER_CTX_free(ctx);

	if (encode_len != ciphertext_calc_size)
		return NULL;

	memcpy(&packet->header.crc, crypt_buffer, SYNCS_CRYPT_HEADER_SIZE);
	memcpy(packet->buffer, crypt_buffer + SYNCS_CRYPT_HEADER_SIZE, encode_len - SYNCS_CRYPT_HEADER_SIZE);

	return packet;
}

struct syncs_packet *syncs_crypt_decode_packet(uint8_t *key, struct syncs_packet *packet)
{
	uint32_t packet_data_size = SYNCS_PACKET_DATA_SIZE(packet->header.data_size);
	uint32_t size = SYNCS_CRYPT_HEADER_SIZE + packet_data_size;

	uint8_t *iv = key;
	uint8_t *decryption_key = key + AES_BLOCK_SIZE;

	EVP_CIPHER_CTX *ctx;
	int len;
	int decode_len;

	memcpy(crypt_buffer, &packet->header.crc, SYNCS_CRYPT_HEADER_SIZE);
	memcpy(crypt_buffer + SYNCS_CRYPT_HEADER_SIZE, packet->buffer, packet_data_size);

	if (!(ctx = EVP_CIPHER_CTX_new()))
		return NULL;

	if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, decryption_key, iv) != 1)
		return NULL;

	if (EVP_DecryptUpdate(ctx, crypt_buffer, &len, crypt_buffer, size) != 1)
		return NULL;
	decode_len = len;

	if (EVP_DecryptFinal_ex(ctx, crypt_buffer + len, &len) != 1)
		return NULL;
	decode_len += len;

	memcpy(&packet->header.crc, crypt_buffer, SYNCS_CRYPT_HEADER_SIZE);
	memcpy(packet->buffer, crypt_buffer + SYNCS_CRYPT_HEADER_SIZE, decode_len - SYNCS_CRYPT_HEADER_SIZE);

	EVP_CIPHER_CTX_free(ctx);
	return packet;
}

#endif
