/*
 * Copyright (c) 2016-2017, Linaro Limited
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef TA_LUA_RUNTIME_H
#define TA_LUA_RUNTIME_H


/*
 * This UUID is generated with uuidgen
 * the ITU-T UUID generator at http://www.itu.int/ITU-T/asn1/uuid.html
 */
#define TA_LUA_RUNTIME_UUID \
	{ 0xdebd5a03, 0xe1c1, 0x4e16, \
		{ 0x89, 0xa9, 0xc2, 0x94, 0xe3, 0xd7, 0x8c, 0xd5} }

#define BYTE_BUFFER_SIZE 8192


/* The function IDs implemented in this TA */
#define TA_RUN_LUA_SCRIPT		1
#define TA_RUN_SAVED_LUA_SCRIPT		2
#define TA_SAVE_LUA_SCRIPT		3
#define TA	4

#define LUA_MODE_PLAINTEXT	0
#define LUA_MODE_ENCRYPTED	1

#define LUA_TYPE_NUMBER 0
#define LUA_TYPE_STRING 1
#define LUA_TYPE_CODE 2

#endif /*TA_LUA_RUNTIME_H*/
