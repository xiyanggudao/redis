/* quicklist.h - A generic doubly linked quicklist implementation
 *
 * Copyright (c) 2014, Matt Stancliff <matt@genges.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this quicklist of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this quicklist of conditions and the following disclaimer in the
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

// quicklist最基本双向（不循环）链表实现，adlist也是双向（不循环）链表，
// 区别是这里的list一个节点有多个对象，实际上是块状链表，而且节点可能是经过压缩，
// 数据存储更紧凑，而adlist只是最简单的双向（不循环）链表实现
#ifndef __QUICKLIST_H__
#define __QUICKLIST_H__

/* Node, quicklist, and Iterator are the only data structures used currently. */

/* quicklistNode is a 32 byte struct describing a ziplist for a quicklist.
 * We use bit fields keep the quicklistNode at 32 bytes.
 * count: 16 bits, max 65536 (max zl bytes is 65k, so max count actually < 32k).
 * encoding: 2 bits, RAW=1, LZF=2.
 * container: 2 bits, NONE=1, ZIPLIST=2.
 * recompress: 1 bit, bool, true if node is temporarry decompressed for usage.
 * attempted_compress: 1 bit, boolean, used for verifying during testing.
 * extra: 10 bits, free for future use; pads out the remainder of 32 bits */
// 链表节点
typedef struct quicklistNode {
    // 前后节点指针
    struct quicklistNode *prev;
    struct quicklistNode *next;
    // 指向ziplist或者quicklistLZF，类型由encoding字段决定
    unsigned char *zl;
    // ziplist的占用空间，没压缩的情况下可以直接从zl得到，
    // 但是经过LZF压缩后这个字段会比较有用，解压时也可以分配恰当的内存
    unsigned int sz;             /* ziplist size in bytes */
    // 存储在ziplist的元素个数，也是节点里的元素个数，
    // 在没压缩的情况下可以直接从zl得到
    unsigned int count : 16;     /* count of items in ziplist */
    // zl指向内容类型，ziplist或者经过LZF压缩过的ziplist
    unsigned int encoding : 2;   /* RAW==1 or LZF==2 */
    // zl指向的容器默认是ziplist，看实现也只有ziplist一种
    unsigned int container : 2;  /* NONE==1 or ZIPLIST==2 */
    // 是否节点曾经被压缩过，节点解压之后字段置1
    unsigned int recompress : 1; /* was this node previous compressed? */
    // 看实现attempted_compress似乎只在测试代码里用到，
    // 每次节点尝试压缩这个字段就会置1，尝试解压字段就会置0
    unsigned int attempted_compress : 1; /* node can't compress; too small */
    // 预留位
    unsigned int extra : 10; /* more bits to steal for future usage */
} quicklistNode;

/* quicklistLZF is a 4+N byte struct holding 'sz' followed by 'compressed'.
 * 'sz' is byte length of 'compressed' field.
 * 'compressed' is LZF data with total (compressed) length 'sz'
 * NOTE: uncompressed length is stored in quicklistNode->sz.
 * When quicklistNode->zl is compressed, node->zl points to a quicklistLZF */
// sizeof(quicklistLZF) == 4，长度为0的数组不占内存
typedef struct quicklistLZF {
    unsigned int sz; /* LZF size in bytes*/
    // 长度为0的数组不占内存，奇妙的用法
    char compressed[];
} quicklistLZF;

/* quicklist is a 40 byte struct (on 64-bit systems) describing a quicklist.
 * 'count' is the number of total entries.
 * 'len' is the number of quicklist nodes.
 * 'compress' is: -1 if compression disabled, otherwise it's the number
 *                of quicklistNodes to leave uncompressed at ends of quicklist.
 * 'fill' is the user-requested (or default) fill factor. */
// 双向链表
typedef struct quicklist {
    // 链表的头尾节点指针
    quicklistNode *head;
    quicklistNode *tail;
    // 链表存储的元素个数
    unsigned long count;        /* total count of all entries in all ziplists */
    // 链表的节点个数，一个节点可以存储多个元素
    unsigned long len;          /* number of quicklistNodes */
    // 控制节点ziplist占用空间上限的字段，目前实现的取值范围是[-5, 2^15]，
    // 作为数组的索引，[-5, 0)映射的索引为[0, 5)，如果fill>=0代表没有上限
    // 对应的ziplist占用空间上限为4096, 8192, 16384, 32768, 65536，
    // 默认的上限是8192
    // ??? 不清楚为什么fill字段取负值，有点别扭
    int fill : 16;              /* fill factor for individual nodes */
    // 压缩深度，链表两头compress个节点不会经过LZF压缩，链表两头的元素访问比较频繁，
    // 不压缩有利于性能，中间的节点压缩可以节省内存
    // 如果compress为0（看实现是0不是-1，上面英文注释有误），所有节点都不会经过LZF压缩
    unsigned int compress : 16; /* depth of end nodes not to compress;0=off */
} quicklist;

typedef struct quicklistIter {
    const quicklist *quicklist;
    quicklistNode *current;
    unsigned char *zi;
    long offset; /* offset in current ziplist */
    int direction;
} quicklistIter;

typedef struct quicklistEntry {
    const quicklist *quicklist;
    quicklistNode *node;
    unsigned char *zi;
    unsigned char *value;
    long long longval;
    unsigned int sz;
    int offset;
} quicklistEntry;

#define QUICKLIST_HEAD 0
#define QUICKLIST_TAIL -1

/* quicklist node encodings */
#define QUICKLIST_NODE_ENCODING_RAW 1
#define QUICKLIST_NODE_ENCODING_LZF 2

/* quicklist compression disable */
#define QUICKLIST_NOCOMPRESS 0

/* quicklist container formats */
#define QUICKLIST_NODE_CONTAINER_NONE 1
#define QUICKLIST_NODE_CONTAINER_ZIPLIST 2

#define quicklistNodeIsCompressed(node)                                        \
    ((node)->encoding == QUICKLIST_NODE_ENCODING_LZF)

/* Prototypes */
// 新创建quicklist
quicklist *quicklistCreate(void);
// 新创建quicklist，指定fill和压缩深度compress
quicklist *quicklistNew(int fill, int compress);
// 设置压缩深度compress字段，这里只是修改compress，
// 并不会改变实际节点的压缩状态，
// 所以应该只有链表刚创建里面还没有元素的时候才能调用这个接口
void quicklistSetCompressDepth(quicklist *quicklist, int depth);
void quicklistSetFill(quicklist *quicklist, int fill);
void quicklistSetOptions(quicklist *quicklist, int fill, int depth);
// 释放链表内存，包括所有的节点和链表本身
void quicklistRelease(quicklist *quicklist);
int quicklistPushHead(quicklist *quicklist, void *value, const size_t sz);
int quicklistPushTail(quicklist *quicklist, void *value, const size_t sz);
void quicklistPush(quicklist *quicklist, void *value, const size_t sz,
                   int where);
void quicklistAppendZiplist(quicklist *quicklist, unsigned char *zl);
quicklist *quicklistAppendValuesFromZiplist(quicklist *quicklist,
                                            unsigned char *zl);
quicklist *quicklistCreateFromZiplist(int fill, int compress,
                                      unsigned char *zl);
void quicklistInsertAfter(quicklist *quicklist, quicklistEntry *node,
                          void *value, const size_t sz);
void quicklistInsertBefore(quicklist *quicklist, quicklistEntry *node,
                           void *value, const size_t sz);
void quicklistDelEntry(quicklistIter *iter, quicklistEntry *entry);
int quicklistReplaceAtIndex(quicklist *quicklist, long index, void *data,
                            int sz);
int quicklistDelRange(quicklist *quicklist, const long start, const long stop);
quicklistIter *quicklistGetIterator(const quicklist *quicklist, int direction);
quicklistIter *quicklistGetIteratorAtIdx(const quicklist *quicklist,
                                         int direction, const long long idx);
int quicklistNext(quicklistIter *iter, quicklistEntry *node);
void quicklistReleaseIterator(quicklistIter *iter);
quicklist *quicklistDup(quicklist *orig);
int quicklistIndex(const quicklist *quicklist, const long long index,
                   quicklistEntry *entry);
void quicklistRewind(quicklist *quicklist, quicklistIter *li);
void quicklistRewindTail(quicklist *quicklist, quicklistIter *li);
void quicklistRotate(quicklist *quicklist);
int quicklistPopCustom(quicklist *quicklist, int where, unsigned char **data,
                       unsigned int *sz, long long *sval,
                       void *(*saver)(unsigned char *data, unsigned int sz));
int quicklistPop(quicklist *quicklist, int where, unsigned char **data,
                 unsigned int *sz, long long *slong);
// 返回链表内存储的元素个数
unsigned long quicklistCount(const quicklist *ql);
int quicklistCompare(unsigned char *p1, unsigned char *p2, int p2_len);
// 获取节点经过LZF压缩的内容，返回内容的长度，*data输出内容首地址，
// 函数不会检测节点是否是压缩过的，使用的地方需要保证这一点
size_t quicklistGetLzf(const quicklistNode *node, void **data);

#ifdef REDIS_TEST
int quicklistTest(int argc, char *argv[]);
#endif

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __QUICKLIST_H__ */
