/* adlist.h - A generic doubly linked list implementation
 *
 * Copyright (c) 2006-2012, Salvatore Sanfilippo <antirez at gmail dot com>
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

// adlist最基本双向（不循环）链表实现，quicklist也是双向（不循环）链表，
// 区别是这里的list一个节点只有一个对象，而quicklist实际上是块状链表，
// 一个节点中有多个对象，而且quicklist的节点可能是经过压缩的，数据存储更紧凑
#ifndef __ADLIST_H__
#define __ADLIST_H__

/* Node, List, and Iterator are the only data structures used currently. */

// 链表节点
typedef struct listNode {
    struct listNode *prev;
    struct listNode *next;
    // 节点中引用的对象
    void *value;
} listNode;

// 用以遍历链表的迭代器
typedef struct listIter {
    listNode *next;
    // 遍历方向，顺序和逆序两种（AL_START_HEAD 0/AL_START_TAIL 1）
    int direction;
} listIter;

// 链表对象
typedef struct list {
    // 头尾指针，如果是循环链表的话，感觉有头指针就行，
    // 尾指针就是head->prev，不过这种实现细节区别不大
    listNode *head;
    listNode *tail;
    // Duplicate函数，提供对象复制操作，如果为空，则链表复制时是指针复制
    void *(*dup)(void *ptr);
    // 对象释放操作，删除链表节点时会释放节点引用的对象，
    // 如果为空，则链表释放时不会释放对象
    void (*free)(void *ptr);
    // 对象查找时会用到的匹配操作，如果为空，则查找时是指针地址比较
    int (*match)(void *ptr, void *key);
    // 链表里的对象个数
    unsigned long len;
} list;

/* Functions implemented as macros */
#define listLength(l) ((l)->len)
#define listFirst(l) ((l)->head)
#define listLast(l) ((l)->tail)
#define listPrevNode(n) ((n)->prev)
#define listNextNode(n) ((n)->next)
#define listNodeValue(n) ((n)->value)

#define listSetDupMethod(l,m) ((l)->dup = (m))
#define listSetFreeMethod(l,m) ((l)->free = (m))
#define listSetMatchMethod(l,m) ((l)->match = (m))

#define listGetDupMethod(l) ((l)->dup)
#define listGetFree(l) ((l)->free)
#define listGetMatchMethod(l) ((l)->match)

/* Prototypes */
// 创建链表
list *listCreate(void);
// 释放链表
void listRelease(list *list);
// 清空链表，链表对象本身仍在，节点被清除
void listEmpty(list *list);
// 链表头部插入节点
list *listAddNodeHead(list *list, void *value);
// 链表尾部插入节点
list *listAddNodeTail(list *list, void *value);
// 链表指定位置插入节点
// after如果为true则在old_node之后的位置插入节点，否则在old_node之前插入节点
list *listInsertNode(list *list, listNode *old_node, void *value, int after);
// 删除节点
void listDelNode(list *list, listNode *node);
// 创建链表的迭代器
// direction是迭代器遍历的方向（AL_START_HEAD从头到尾/AL_START_TAIL从尾到头）
listIter *listGetIterator(list *list, int direction);
// 迭代器移动到下一个位置，返回当前节点
listNode *listNext(listIter *iter);
// 释放迭代器
void listReleaseIterator(listIter *iter);
// 链表复制
list *listDup(list *orig);
// 通过链表的match方法查找节点
listNode *listSearchKey(list *list, void *key);
// 获取第index个节点，如果index为负，这是从后往前第|index|个节点
listNode *listIndex(list *list, long index);
// 重置迭代器到链表头部，向尾部方向遍历
void listRewind(list *list, listIter *li);
// 重置迭代器到链表尾部，向头部方向遍历
void listRewindTail(list *list, listIter *li);
// 旋转链表，把尾部节点移到头部
void listRotate(list *list);
// 链表合并操作，o合并到l，合并之后o为空链表
void listJoin(list *l, list *o);

/* Directions for iterators */
#define AL_START_HEAD 0
#define AL_START_TAIL 1

#endif /* __ADLIST_H__ */
