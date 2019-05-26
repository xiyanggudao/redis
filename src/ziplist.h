/*
 * Copyright (c) 2009-2012, Pieter Noordhuis <pcnoordhuis at gmail dot com>
 * Copyright (c) 2009-2012, Salvatore Sanfilippo <antirez at gmail dot com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _ZIPLIST_H
#define _ZIPLIST_H

#define ZIPLIST_HEAD 0
#define ZIPLIST_TAIL 1

// 新创建一个空列表，只包含列表头尾，没有多余的内存，返回列表指针
unsigned char *ziplistNew(void);
// 两个ziplist合并，second追加到first
// 如果合并成功，返回合并后的ziplist，合并完成后释放另一个列表，
// first和second占用空间大的会置为合并后的列表地址，另一个置空，
// 如果合并失败，返回nullptr
unsigned char *ziplistMerge(unsigned char **first, unsigned char **second);
// 在起始或者末尾位置添加元素(s, slen)，返回列表地址（列表地址可能发生改变）
// 起始端添加元素应该是O(n)复杂度，末尾端添加元素应该是O(1)复杂度
unsigned char *ziplistPush(unsigned char *zl, unsigned char *s, unsigned int slen, int where);
// 返回第index元素位置，如果没有第index个元素，返回nullptr
// index为正是下标，为负是倒数的下标，-1是最后一个元素
unsigned char *ziplistIndex(unsigned char *zl, int index);
// 获取p之后一个元素的地址，如果没有后一个元素，返回nullptr
// p是zltail也返回nullptr
unsigned char *ziplistNext(unsigned char *zl, unsigned char *p);
// 获取p之前一个元素的地址，如果没有前一个元素，返回nullptr
// p可以是zlend
unsigned char *ziplistPrev(unsigned char *zl, unsigned char *p);
// 获取p位置的元素，如果是整数类型则存储在lval，如果是字符串类型则存储在(sval, slen)
// 获取成功返回1，获取失败返回0
unsigned int ziplistGet(unsigned char *p, unsigned char **sval, unsigned int *slen, long long *lval);
// 在指定位置p之前插入元素(s, slen)，返回列表地址
// 插入过程可能会有内存的重新分配，返回的地址不一定和zl一致
unsigned char *ziplistInsert(unsigned char *zl, unsigned char *p, unsigned char *s, unsigned int slen);
// 删除元素*p（*p指向zl的元素位置），删除元素之后*p指向下一个元素，返回删除列表指针
// 删除过程可能会有内存的重新分配，返回的地址不一定和zl一致
unsigned char *ziplistDelete(unsigned char *zl, unsigned char **p);
// 从第index个元素开始，向后删除最多num个元素，返回删除列表指针
// 删除过程可能会有内存的重新分配，返回的地址不一定和zl一致
unsigned char *ziplistDeleteRange(unsigned char *zl, int index, unsigned int num);
// 比较元素p和(s, slen)是否相等，如果相等返回1，不相等返回0
unsigned int ziplistCompare(unsigned char *p, unsigned char *s, unsigned int slen);
// 查找与(vstr, vlen)相等的元素，从p开始，skip是每次迭代跳过的元素个数，skip为0则是查找后续所有
// 如果找到匹配的元素，返回元素地址，否则返回nullptr
unsigned char *ziplistFind(unsigned char *p, unsigned char *vstr, unsigned int vlen, unsigned int skip);
// 返回zl元素个数
unsigned int ziplistLen(unsigned char *zl);
// 返回ziplist占用的内存大小
size_t ziplistBlobLen(unsigned char *zl);
// 把ziplist的内容输出到控制台，可能是调试或者log用的
void ziplistRepr(unsigned char *zl);

#ifdef REDIS_TEST
int ziplistTest(int argc, char *argv[]);
#endif

#endif /* _ZIPLIST_H */
