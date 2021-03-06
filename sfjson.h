#ifndef _SFJSON_H__
#define _SFJSON_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <string>
#include <algorithm>

#ifdef _WINDOWS
#	undef min
#	undef max
#	include <stdint.h>
#else

#endif

// 编码时的初始化内存块大小
#define SFJ_ENCODE_INIT_MEMSIZE	4096
// JSON的解析深度如果超过这个值，则会被报错
#define SFJ_MAX_PARSE_LEVEL		200

#define SFJ_SKIP_WHITES()\
	while(pReadPos != m_pMemEnd) {\
		uint8_t ch = pReadPos[0];\
		if (ch > 32)\
			break;\
		if (json_invisibles_allowed[ch] != 1) {\
			m_iErr = kErrorSymbol;\
			return 0;\
		}\
		pReadPos ++;\
	}\
	if (pReadPos >= m_pMemEnd) return 0;

#define SFJ_SKIP_TO(dst)\
	m_pLastPos = pReadPos;\
	while(pReadPos != m_pMemEnd) {\
		uint8_t ch = pReadPos[0];\
		if (ch > 32 || ch == dst)\
			break;\
		if (json_invisibles_allowed[ch] != 1) {\
			m_iErr = kErrorSymbol;\
			return 0;\
		}\
		pReadPos ++;\
	}\
	if (pReadPos >= m_pMemEnd) return 0;


enum sfNodeValueType
{
	JATNone,
	JATIntValue,
	JATDoubleValue,
	JATBooleanValue,
	JATNullValue,
	JATString,
	JATObject,
	JATArray,
};

enum sfJsonFlags {
	kJsonUnicodes = 1,
	kJsonSimpleEscape = 2,
};

class sfNode;

// Json的Node
/*
 * 使用全局函数：sfJsonDecode 来从字符串创建出Json Node对象，即可使用
 * 使用全局函数：sfJsonCreate 创建出一个Root节点，然后就可以使用add函数来添加层次
 *
 * decode示例：
 *   sfNode* root = sfJsonDecode(jsonString, stringLength);
 *   puts(root->find("name"));
 *   puts(root->index(0)->strval);
 *   推荐不使用index函数，因为他每次都需要从链表头开始迭代，最好是根据代码上下文的需要，自行从child开始向后使用next成员逐个操作
 *
 * encode示例：
 *   sfNode* root = sfJsonCreate(true);
 *   链式操作，一次性添加多个：
 *   root->addValue("name", "xiaomi")->addValue("type", "phone")->addValue("price", 999)
 *   一个更完整的链式操作
		n = sfJsonCreate(true);
		n->add(
			n->createArray()->append((int)1)->append(3.5f)->append(true), "group1"
		)->add(
			n->createObject("group2")->appendNamed("a", 3.5)->appendNamed("b", false)->appendNamed("c")->add(
				n->createArray()->add(n->createValue()->val("Hello World!")), "last"
			)
		);
 *
 *   encode时函数名的规则：
 *      1、create XXXX系列的函数只是创建一个节点，并且返回创建出的节点
 *      2、add XXXX系列函数用于创建并将该节点添加为自己的子节点，返回为新创建的节点
 *      3、append XXXX和appendNamed XXXX系列函数用于往自己的子节点中追加一个新的值类型的子节点（或命名的子节点）同时返回this，因此可以链式调用
 *      4、name和val函数只是对名字和值进行设置
 *
 */
class sfNodeBase
{
public:
	char			*nameKey;
	union {
		bool		bval;
		int64_t		ival;
		int32_t		i32val;
		double		dval;
		char		*strval;
	};

	uint32_t		nameLength;
	uint32_t		valLength;						// 编码时，-1表示非字符串值，否则表示解码时从源码字符串读出的值的字符个数

	uint32_t		childCount;
	uint32_t		nodeType;

	sfNode			*child, *childLast, *next;
	void			*pFile;
};

namespace sfjson {

	// 下面是ASCII字符属性表，1表示符号，2表示大小写字母，3表示数字，4表示可以用于组合整数或小数的符号
	// 1 = 0~9
	// 2 = + - .
	// 3 = , ] }
	// 4&5 = a-z
	// 6&7 = A-Z
	static uint8_t json_value_char_tbl[128] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	0,		// 0~32
		0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 2, 3, 2, 2, 0,	// 33~47
		1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	// 48~57
		0, 0, 0, 0, 0, 0, 0,	// 58~64
		6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	// 65~92
		0, 0, 3, 0, 0, 0,	// 91~96
		4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,	// 97~122
		0, 0, 3, 0, 0,
	};
	static uint8_t json_allends_allowed[128] = { 0 };
	static uint8_t json_invisibles_allowed[33] =
	{
		0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1
	};

	static uint8_t json_escape_chars[256] = { 0 };
	static uint8_t json_unescape_chars[256] = { 0 };

	static void initJsonEscapeChars()
	{
		if (json_escape_chars['\\'])
			return ;

		json_allends_allowed[','] = json_allends_allowed['}'] = json_allends_allowed[']'] = json_allends_allowed[' '] = 1;

		json_escape_chars['\\'] = 1;
		//json_escape_chars['/'] = 1;
		json_escape_chars['"'] = 1;
		json_escape_chars['\\'] = 1;
		json_escape_chars['\n'] = 'n';
		json_escape_chars['\r'] = 'r';
		json_escape_chars['\t'] = 't';
		json_escape_chars['\a'] = 'a';
		json_escape_chars['\f'] = 'f';
		json_escape_chars['\v'] = 'v';
		json_escape_chars['\b'] = 'b';

		json_unescape_chars['n'] = '\n';
		json_unescape_chars['r'] = '\r';
		json_unescape_chars['t'] = '\t';
		json_unescape_chars['a'] = '\a';
		json_unescape_chars['f'] = '\f';
		json_unescape_chars['v'] = '\v';
		json_unescape_chars['b'] = '\b';

		json_unescape_chars['u'] = 'u';
		json_unescape_chars['/'] = '/';
		json_unescape_chars['\\'] = '\\';
		json_unescape_chars['\''] = '\'';
		json_unescape_chars['"'] = '"';
	}

	enum sfJSONValueType
	{
		JVTNone,
		JVTDecimal,
		JVTHex,
		JVTOctal,
		JVTDouble,
		JVTTrue,
		JVTFalse,
		JVTNull,
	};

	class sfJSONString
	{
	public:
		inline sfJSONString()
			: pString(0), nLength(0)
		{
		}
		inline sfJSONString(char* p, int i)
			: pString(p), nLength(i)
		{
		}
		inline sfJSONString(char* p, char* pEnd)
			: pString(p), nLength((int)(pEnd - p))
		{
		}

		inline void empty()
		{
			pString = 0;
			nLength = 0;
		}

		char				*pString;
		size_t				nLength;		
		sfJSONValueType		kType;
		union {
			double			dbl;
			int64_t			i64;
		};
	};

	class MemNode
	{
	public:
		size_t		used;
		size_t		total;
		MemNode		*next;

		inline operator char* () { return (char*)(this + 1); }
		inline operator const char* () { return (const char*)(this + 1); }

		inline operator void* () { return (void*)(this + 1); }
		inline operator const void* () { return (const void*)(this + 1); }
	};

	const size_t MemNodeSIZE = 8192 - sizeof(MemNode);

	class MemList
	{
	public:
		MemNode		*first, *last;

	public:
		inline MemList() 
			: first(NULL), last(NULL)
		{}
		~MemList()
		{
			MemNode* n = first, *nn;
			if (n)
			{
				n = n->next;
				while(n)
				{
					nn = n->next;
					free(n);
					n = nn;
				}
			}
		}

		MemNode* newNode(size_t size = MemNodeSIZE)
		{
			MemNode* n = (MemNode*)malloc(size + sizeof(MemNode));
			new (n) MemNode();

			n->next = NULL;
			n->total = size;
			n->used = 0;

			if (first)
				last->next = n; 
			else
				first = n;
			last = n;
			return n;
		}
		MemNode* wrapNode(char* buf, size_t fixedBufSize)
		{
			assert(fixedBufSize >= sizeof(MemNode) + 16);

			MemNode* n = (MemNode*)buf;
			new (n) MemNode();
			n->total = fixedBufSize - sizeof(MemNode);
			n->used = 0;

			if (first)
				last->next = n;
			else
				first = n;
			last = n;
			return n;
		}

		char* addChar(char ch)
		{
			MemNode* n = (MemNode*)last;
			if (n->used >= n->total)
				n = newNode();

			char* ptr = (char*)(n + 1);
			ptr[n->used ++] = ch;

			return ptr;
		}
		char* addChar2(char ch1, char ch2)
		{
			MemNode* n = (MemNode*)last;
			if (n->used + 1 >= n->total)
				n = newNode();

			char* ptr = (char*)(n + 1);
			size_t used = n->used;
			ptr[used] = ch1;
			ptr[used + 1] = ch2;
			n->used += 2;

			return ptr;
		}
		char* addString(const char* s, size_t len)
		{
			MemNode* n = (MemNode*)last;
			char* ptr = (char*)(n + 1);

			size_t copy = std::min(n->total - n->used, len);
			memcpy(ptr += n->used, s, copy);
			len -= copy;
			n->used += copy;

			if (len > 0)
			{
				// 剩下的直接一次分配够
				n = newNode(std::max(MemNodeSIZE, len));
				ptr = (char*)(n + 1) + n->used;
				memcpy(ptr, s + copy, len);
				n->used += len;
			}

			return ptr;
		}

		void escapeString(const char* src, size_t len, uint32_t flags)
		{
			uint8_t v;
			uint32_t unicode;
			size_t i = 0, spos = 0;
			const char upperChars[] = { "0123456789ABCDEF" };

			while (i < len)
			{
				uint8_t ch = src[i];
				if (ch == 0)
				{
					// 为什么会出现NULL字符呢????
					if (i > spos)
						addString(src + spos, i - spos);
					spos = ++ i;
					continue;
				}

				if (!(flags & kJsonSimpleEscape))
					v = json_escape_chars[ch];
				else if (ch == '\\' || ch == '"')
					v = 1;
				else
					v = 0;

				if (v == 0)
				{
					// defered
					++ i;
					continue;
				}

				if (v == 1)
				{
					// escape some chars
					if (i > spos)
						addString(src + spos, i - spos);
					addChar2('\\', ch);
					spos = ++ i;
				}
				else if (v == 2)
				{
					if (flags & kJsonUnicodes)
					{
						// defered
						if ((ch & 0xE0) == 0xC0)		//2 bit count
							i += 2;
						else if ((ch & 0xF0) == 0xE0)	//3 bit count
							i += 3;
						else if ((ch & 0xF8) == 0xF0)	//4 bit count
							i += 4;
						else
							i ++;

						continue;
					}

					if (i > spos)
						addString(src + spos, i - spos);

					// check utf8
					uint8_t* utf8src = (uint8_t*)src + i;
					if ((ch & 0xE0) == 0xC0)
					{
						//2 bit count
						unicode = ch & 0x1F;
						unicode = (unicode << 6) | (utf8src[1] & 0x3F);
						i += 2;
					}
					else if ((ch & 0xF0) == 0xE0)
					{
						//3 bit count
						unicode = ch & 0xF;
						unicode = (unicode << 6) | (utf8src[1] & 0x3F);
						unicode = (unicode << 6) | (utf8src[2] & 0x3F);
						i += 3;
					}
					else if ((ch & 0xF8) == 0xF0)
					{
						//4 bit count
						unicode = ch & 0x7;
						unicode = (unicode << 6) | (utf8src[1] & 0x3F);
						unicode = (unicode << 6) | (utf8src[2] & 0x3F);
						unicode = (unicode << 6) | (utf8src[3] & 0x3F);
						i += 4;
					}
					else
					{
						unicode = '?';
						i ++;
						assert(0);
					}

					char* utf8dst = reserve(6);
					utf8dst[0] = '\\';
					utf8dst[1] = 'u';

					utf8dst[2] = upperChars[(unicode >> 12) & 0xF];
					utf8dst[3] = upperChars[(unicode >> 8) & 0xF];
					utf8dst[4] = upperChars[(unicode >> 4) & 0xF];
					utf8dst[5] = upperChars[unicode & 0xF];

					spos = i;
				}
				else
				{
					// invisible(s) to visibled
					if (i > spos)
						addString(src + spos, i - spos);
					addChar2('\\', v);
					spos = ++ i;
				}
			}

			if (i > spos)
				addString(src + spos, i - spos);
		}

		char* reserve(size_t len)
		{
			char* ptr;
			MemNode* n = (MemNode*)last;

			// 最后一个节点剩下的空间不够需要保留的话，就直接分配一个全新的至少有len这么大的
			if (n->used + len < n->total)
			{
				ptr = (char*)(n + 1);
				ptr += n->used;
			}
			else
			{
				n = newNode(std::max(MemNodeSIZE, len));
				ptr = (char*)(n + 1);
			}

			n->used += len;
			return ptr;
		}
	};

	template <typename T, size_t BlockSize = 4096> class TMemoryPool
	{
	public:
		typedef T               value_type;
		typedef T*              pointer;
		typedef size_t          size_type;

		TMemoryPool() throw()
		{
			currentBlock_ = 0;
			currentSlot_ = 0;
			lastSlot_ = 0;
			freeSlots_ = 0;
			slotsCount_ = 0;
		}

		inline ~TMemoryPool() throw()
		{
			freeall();
		}

		inline pointer allocate(size_t cc = 1)
		{
			assert(cc == 1);

			slotsCount_ ++;
			if (freeSlots_ != 0) {
				pointer result = reinterpret_cast<pointer>(freeSlots_);
				freeSlots_ = freeSlots_->next;
				return result;
			}

			if (currentSlot_ >= lastSlot_)
				allocateBlock();
			return reinterpret_cast<pointer>(currentSlot_++);
		}
		inline void deallocate(pointer p, size_type cc = 0)
		{
			if (p != 0) {
				slotsCount_ --;
				reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
				freeSlots_ = reinterpret_cast<slot_pointer_>(p);
			}
		}

	private:
		union Slot_ {
			char objspace[sizeof(value_type)];
			Slot_* next;
		};

		typedef char* data_pointer_;
		typedef Slot_ slot_type_;
		typedef Slot_* slot_pointer_;

		slot_pointer_ currentBlock_;
		slot_pointer_ currentSlot_;
		slot_pointer_ lastSlot_;
		slot_pointer_ freeSlots_;
		size_type	  slotsCount_;

		size_type padPointer(data_pointer_ p, size_type align) const throw()
		{
			size_t result = reinterpret_cast<size_t>(p);
			return ((align - result) % align);
		}

		void allocateBlock(data_pointer_ newBlock = 0)
		{
			if (!newBlock)
				newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));

			reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_;
			currentBlock_ = reinterpret_cast<slot_pointer_>(newBlock);

			data_pointer_ body = newBlock + sizeof(slot_pointer_);
			size_type bodyPadding = padPointer(body, sizeof(slot_type_));
			currentSlot_ = reinterpret_cast<slot_pointer_>(body + bodyPadding);
			lastSlot_ = reinterpret_cast<slot_pointer_>(newBlock + BlockSize - sizeof(slot_type_) + 1);
		}

		void freeall()
		{
			slot_pointer_ curr = currentBlock_;
			while (curr != 0) {
				slot_pointer_ prev = curr->next;
				operator delete(reinterpret_cast<void*>(curr));
				curr = prev;
			}

			currentBlock_ = 0;
			currentSlot_ = 0;
			lastSlot_ = 0;
			freeSlots_ = 0;
			slotsCount_ = 0;
		}
	};

	//////////////////////////////////////////////////////////////////////////

	// JSON File Object
	class JFile
	{
		friend class ::sfNode;
	public:
		enum
		{
			kErrorNotClosed = 1,
			kErrorEnd,
			kErrorAttribute,
			kErrorCloseNode,
			kErrorName,
			kErrorSymbol,
			kErrorMaxDeeps,
			kErrorValue
		};

	private:
		typedef TMemoryPool<sfNodeBase> NodesPool;

		char				*m_pMemEnd;
		char				*m_pLastPos;
		size_t				m_nMemSize;
		int					m_iErr;
		sfJSONValueType		kValType;

		NodesPool			m_nodesPool;
		MemList				m_strBuf;
		uint32_t			m_nOpens;
		uint32_t			m_opens[SFJ_MAX_PARSE_LEVEL];
		sfNodeBase			*m_nodeOpens[SFJ_MAX_PARSE_LEVEL];
		sfNodeBase			*m_pRoot;

	public:
		inline JFile(size_t nSize)
			: m_nMemSize(nSize)
			, m_pLastPos(NULL)
			, m_iErr(0)
			, m_nOpens(0)
			, m_pRoot(NULL)
		{
			initJsonEscapeChars();
		}

		inline JFile(char* init, size_t nSize)
			: m_nMemSize(0)
			, m_pLastPos(NULL)
			, m_iErr(0)
			, m_nOpens(0)
			, m_pRoot(NULL)
		{
			initJsonEscapeChars();
			m_strBuf.wrapNode(init, nSize);
		}

		~JFile()
		{
		}

		inline sfNode* getRoot()
		{
			m_pRoot->pFile = this;
			return (sfNode*)m_pRoot;
		}

		sfNode* newRoot(bool bIsObject)
		{
			m_pRoot = m_nodesPool.allocate();
			memset(m_pRoot, 0, sizeof(sfNodeBase));

			m_pRoot->pFile = this;
			m_pRoot->nodeType = bIsObject ? JATObject : JATArray;

			return (sfNode*)m_pRoot;
		}

		size_t parse(char* pMemory)
		{
			if (!pMemory)
				pMemory = (char*)(this + 1);
			m_pMemEnd = pMemory + m_nMemSize;

			char* pReadPos = parseRoot(m_pLastPos = pMemory);
			if (!pReadPos)
				return 0;

			if (m_nOpens)
			{
				//未正常关闭
				m_iErr = kErrorNotClosed;
				return 0;
			}

			return pReadPos - pMemory;
		}

		const char* getError()
		{
			switch (m_iErr)
			{
			case kErrorNotClosed:
			{
				static char msg[] = { "node not closed" };
				return msg;
			}
			break;

			case kErrorEnd:
			{
				static char msg[] = { "error end" };
				return msg;
			}
			break;

			case kErrorAttribute:
			{
				static char msg[] = { "error attribute or value" };
				return msg;
			}
			break;

			case kErrorCloseNode:
			{
				static char msg[] = { "incorrected close node" };
				return msg;
			}
			break;

			case kErrorName:
			{
				static char msg[] = { "name with no data" };
				return msg;
			}
			break;

			case kErrorSymbol:
			{
				static char msg[] = { "unrecognize/invalid symbol" };
				return msg;
			}
			break;

			case kErrorMaxDeeps:
			{
				static char msg[] = { "exceed max parse deepths" };
				return msg;
			}
			break;

			case kErrorValue:
			{
				static char msg[] = { "special value must be one of true|false|null" };
				return msg;
			}
			break;
			}

			return "";
		}

		size_t summary(char* outs, size_t maxl)
		{
			size_t bytes = std::min((size_t)(m_pMemEnd - m_pLastPos), maxl);

			memcpy(outs, m_pLastPos, bytes);
			return bytes;
		}

		void printToBuffer(MemList& encodeBuf, sfNodeBase* node, sfNodeBase* parent, uint32_t flags)
		{			
			size_t len;
			sfNodeBase* n;
			char szBuf[32] = { 0 };

			if (parent->nodeType == JATObject)
			{
				assert(node->nameKey);

				encodeBuf.addChar('"');
				encodeBuf.addString(node->nameKey, node->nameLength);
				encodeBuf.addChar2('"', ':');
			}

			switch (node->nodeType)
			{
			case JATBooleanValue:
				if (node->bval)
					encodeBuf.addString("true", 4);
				else
					encodeBuf.addString("false", 5);
				break;

			case JATIntValue:
#ifdef _SFJSON_CRTOPT_H__
				len = opt_i64toa(node->ival, szBuf);
#else
				len = sprintf(szBuf, "%lld", node->ival);
#endif
				encodeBuf.addString(szBuf, len);
				break;

			case JATDoubleValue:
#ifdef _SFJSON_CRTOPT_H__
				len = opt_dtoa(node->dval, szBuf);
#else
				len = sprintf(szBuf, "%f", node->dval);
#endif
				encodeBuf.addString(szBuf, len);
				break;

			case JATString:
				encodeBuf.addChar('"');
				encodeBuf.escapeString(node->strval, node->valLength, flags);
				encodeBuf.addChar('"');
				break;

			case JATNullValue:
				encodeBuf.addString("null", 4);
				break;

			case JATObject:			
				encodeBuf.addChar('{');
				len = 0;
				n = (sfNodeBase*)node->child;
				while (n)
				{
					if (len ++ > 0)
						encodeBuf.addChar(',');
					printToBuffer(encodeBuf, n, node, flags);
					n = (sfNodeBase*)n->next;
				}
				encodeBuf.addChar('}');
				break;

			case JATArray:
				encodeBuf.addChar('[');
				len = 0;
				n = (sfNodeBase*)node->child;
				while (n)
				{
					if (len ++ > 0)
						encodeBuf.addChar(',');
					printToBuffer(encodeBuf, n, node, flags);
					n = (sfNodeBase*)n->next;
				}
				encodeBuf.addChar(']');
				break;
			}
		}

	private:
		sfNodeBase* openNode(uint32_t kAttr, sfNodeBase* parent)
		{
			sfNodeBase* n = m_nodesPool.allocate();
			memset(n, 0, sizeof(sfNodeBase));

			n->nodeType = kAttr;
			m_opens[m_nOpens] = kAttr;
			m_nodeOpens[m_nOpens ++] = n;

			if (parent)
			{
				parent->childCount ++;
				if (parent->child)
					((sfNodeBase*)parent->childLast)->next = (sfNode*)n;
				else
					parent->child = (sfNode*)n;
				parent->childLast = (sfNode*)n;
			}

			return n;
		}

		sfNodeBase* openNode(uint32_t kAttr, sfJSONString& name, sfNodeBase* parent)
		{
			sfNodeBase* n = openNode(kAttr, parent);

			n->nameKey = name.pString;
			n->nameLength = name.nLength;
			n->nameKey[n->nameLength] = 0;

			return n;
		}

		sfNodeBase* addValueNode(bool bIsString, sfJSONString& val, sfNodeBase* parent)
		{
			sfNodeBase* n = m_nodesPool.allocate();
			memset(n, 0, sizeof(sfNodeBase));

			if (bIsString)
			{
				n->strval = val.pString;
				n->valLength = val.nLength;
				n->nodeType = JATString;
				n->strval[n->valLength] = 0;
			}
			else
			{
				switch (val.kType)
				{
				case JVTTrue:
					n->bval = true;
					n->nodeType = JATBooleanValue;
					break;
				case JVTFalse:
					n->bval = false;
					n->nodeType = JATBooleanValue;
					break;
				case JVTDouble:
					n->dval = val.dbl;
					n->nodeType = JATDoubleValue;
					break;
				case JVTDecimal:
				case JVTOctal:
				case JVTHex:
					n->ival = val.i64;
					n->nodeType = JATIntValue;
					break;
				case JVTNull:
					n->dval = 0;
					n->nodeType = JATNullValue;
					break;
				}
			}

			parent->childCount ++;
			if (parent->child)
				((sfNodeBase*)parent->childLast)->next = (sfNode*)n;
			else
				parent->child = (sfNode*)n;
			parent->childLast = (sfNode*)n;

			return n;
		}

		char* parseRoot(char* pReadPos)
		{
			SFJ_SKIP_WHITES();

			if (pReadPos[0] == '[')
			{
				pReadPos = parseArray(pReadPos + 1, m_pRoot = openNode(JATArray, NULL));
			}
			else if (pReadPos[0] == '{')
			{
				pReadPos = parseObject(pReadPos + 1, m_pRoot = openNode(JATObject, NULL));
			}
			else
			{
				m_iErr = kErrorSymbol;
				return 0;
			}

			return pReadPos;
		}

		char* parseObject(char* pReadPos, sfNodeBase* parent)
		{
			if (m_nOpens == SFJ_MAX_PARSE_LEVEL)
			{
				m_iErr = kErrorMaxDeeps;
				return 0;
			}

			SFJ_SKIP_WHITES();

			// 解析属性					
			char endChar = 0;
			sfJSONString name, val;
			uint32_t cc = 0, eqSyms = 0;			

			m_pLastPos = pReadPos;
			while (pReadPos != m_pMemEnd)
			{
				SFJ_SKIP_WHITES();

				if (pReadPos[0] == '"')
				{
					pReadPos = parseFetchString(pReadPos, name);
					if (!pReadPos)
						return 0;

					//取下一个符号：冒号
					SFJ_SKIP_TO(':');

					pReadPos ++;
					SFJ_SKIP_WHITES();

					//判断是数组还是Object
					if (pReadPos[0] == '[')
					{
						//处理数组
						pReadPos = parseArray(pReadPos + 1, openNode(JATArray, name, parent));
						if (!pReadPos)
							return 0;
					}
					else if (pReadPos[0] == '{')
					{
						//递归子节点
						pReadPos = parseObject(pReadPos + 1, openNode(JATObject, name, parent));
						if (!pReadPos)
							return 0;
					}
					else
					{
						//取值
						bool bQuoteStart;
						pReadPos = parseFetchString(pReadPos, val, &bQuoteStart);
						if (!pReadPos)
							return 0;
						
						sfNodeBase* n = addValueNode(bQuoteStart, val, parent);
						n->nameKey = name.pString;
						n->nameLength = name.nLength;
						n->nameKey[n->nameLength] = 0;
					}

					SFJ_SKIP_WHITES();
					endChar = pReadPos[0];

					cc ++;
				}
				else if (cc == 0 && pReadPos[0] == '}')
				{
					//空的大括号
					endChar = '}';
				}
				else
				{
					m_iErr = kErrorSymbol;
					return 0;
				}

				//取下一个符号，只能是逗号或者}号
				if (endChar == '}')
				{
					if (m_nOpens == 0 || m_opens[m_nOpens - 1] != JATObject)
						return 0;

					m_nOpens --;
					return pReadPos + 1;
				}
				else if (endChar == ',')
				{
					pReadPos ++;
				}
				else
				{
					return 0;
				}
			}

			m_iErr = kErrorNotClosed;
			return pReadPos;
		}

		//读取数组
		char* parseArray(char* pReadPos, sfNodeBase* parent)
		{
			uint32_t cc = 0;

			if (m_nOpens == SFJ_MAX_PARSE_LEVEL)
			{
				m_iErr = kErrorMaxDeeps;
				return 0;
			}

			sfJSONString val;
			m_pLastPos = pReadPos;

			while (pReadPos != m_pMemEnd)
			{
				SFJ_SKIP_WHITES();

				uint8_t ch = pReadPos[0];
				if (ch == '{')
				{
					//一个新的节点的开始
					pReadPos = parseObject(pReadPos + 1, openNode(JATObject, parent));
				}
				else if (ch == '[')
				{
					//一个新的节点的开始
					pReadPos = parseArray(pReadPos + 1, openNode(JATArray, parent));
				}
				else if (ch == ',')
				{
					//还有值
					pReadPos ++;
					continue;
				}
				else if (ch == ']')
				{
					//结束
					if (m_nOpens == 0 || m_opens[m_nOpens - 1] != JATArray)
						return 0;

					m_nOpens --;
					pReadPos ++;

					break;
				}
				else if (ch == '"' || json_value_char_tbl[ch] <= 2 || json_value_char_tbl[ch] >= 4)
				{
					//数值类
					bool bQuoteStart;
					pReadPos = parseFetchString(pReadPos, val, &bQuoteStart);
					if (!pReadPos)
						break;

					addValueNode(bQuoteStart, val, parent);
				}
				else
				{
					m_iErr = kErrorSymbol;
					pReadPos = 0;
				}

				if (!pReadPos)
					break;

				cc ++;
			}

			return pReadPos;
		}

		//从给定位置开始取一个字符串，直到空格或结束符为止。支持双引号字符串和非双引号字符串
		char* parseFetchString(char* pReadPos, sfJSONString& str, bool* pbQuoteStart = NULL)
		{
			char* pStart = str.pString = pReadPos, *pEndPos = 0;
			bool bQuoteStart = pStart[0] == '"';

			if (pbQuoteStart)
				*pbQuoteStart = bQuoteStart;
			m_pLastPos = pReadPos;

			if (bQuoteStart)
			{
				// 字符串型值
				pReadPos ++;
				str.pString = pEndPos = pReadPos;

				size_t i;
				uint8_t ch;
				for (i = 0; i < m_pMemEnd - pReadPos; ++ i)
				{
					ch = pReadPos[i];
					if (ch == 0)
					{
						m_iErr = kErrorEnd;
						return 0;
					}

					if (ch == '"' || ch == '\\')
						break;
				}

				pReadPos += i;
				pEndPos = pReadPos;

				if (ch == '\\')
				{
					// 如果此时ch为\则说明字符串中含有转义符号，因此进入带有转义的字符串的处理过程。否则，上面的处理就直接结束了整个字符串值的获取过程
					while (pReadPos < m_pMemEnd)
					{
						if (ch == '\\')
						{
							uint8_t next = json_unescape_chars[pReadPos[1]];
							if (next == 0)
							{
								m_iErr = kErrorSymbol;
								return 0;
							}

							if (next == 'u')
							{
								// 解Unicode到UTF8
								uint32_t unicode = readUnicode(pReadPos + 2);
								pEndPos = unicode2utf8(unicode, pEndPos);
								pReadPos += 6;
							}
							else
							{
								*pEndPos ++ = next;
								pReadPos += 2;
							}
						}
						else if (ch != '"')
						{
							*pEndPos ++ = ch;
							pReadPos ++;
						}
						else
							break;

						ch = pReadPos[0];
					}
				}

				if (pReadPos >= m_pMemEnd)
				{
					m_iErr = kErrorEnd;
					return 0;
				}
			}
			else
			{
				// 数值或特殊值，先从第1个符号猜测一下可能是什么类型的值
				sfJSONValueType kValType = JVTNone;
				bool bNegativeVal = false;

				uint8_t ch = pReadPos[0];
				if (ch == '+')
				{
					pReadPos ++;
					ch = pReadPos[0];
				}
				else if (ch == '-')
				{
					pReadPos ++;
					ch = pReadPos[0];
					bNegativeVal = true;
				}

				uint8_t ctl = json_value_char_tbl[ch];
				if (ch == '0')
				{
					// 8进制或16进制可能
					pReadPos ++;
					ch = pReadPos[0];
					if (ch == 'x' || ch == 'X')
					{
						// 十六进制整数
						pReadPos ++;
						kValType = JVTHex;
						if (bNegativeVal)
						{
							m_iErr = kErrorValue;
							return 0;
						}
					}
					else if (ch == '.')
					{
						// 小数
						kValType = JVTDouble;
					}
					else
					{
						// 八进制整数
						kValType = JVTOctal;
					}
				}
				else if (ctl == 1)
				{
					// 十进制整数或数字开头的小数
					char* pTest = pReadPos + 1;

					kValType = JVTDecimal;
					while (pTest < m_pMemEnd)
					{
						ch = *pTest ++;
						if (ch < 32 || ch >= 128)
							return 0;
						if (json_allends_allowed[ch])
							break;
						if (json_value_char_tbl[ch] != 1)
						{
							kValType = JVTDouble;
							break;
						}
					}
				}
				else if (ctl >= 4 && ctl <= 7)
				{
					// 特殊值
					if (strncmp(pReadPos, "true", 4) == 0)
					{
						kValType = JVTTrue;
						pReadPos += 4;
					}
					else if (strncmp(pReadPos, "false", 5) == 0)
					{
						kValType = JVTFalse;
						pReadPos += 5;
					}
					else if (strncmp(pReadPos, "null", 4) == 0)
					{
						kValType = JVTNull;
						pReadPos += 4;
					}
					else
					{
						m_iErr = kErrorValue;
						return 0;
					}

					if (!json_allends_allowed[pReadPos[0]])
					{
						m_iErr = kErrorValue;
						return 0;
					}
				}
				else if (ch == '.')
				{
					// 浮点数
					kValType = JVTDouble;
				}

				// 转换出数值
				str.kType = kValType;
				switch (kValType)
				{
				case JVTDecimal:
					str.i64 = strtoull(pReadPos, &pReadPos, 10);
					break;
				case JVTHex:
					str.i64 = strtoull(pReadPos, &pReadPos, 16);
					break;
				case JVTOctal:
					str.i64 = strtoull(pReadPos, &pReadPos, 8);
					break;
				case JVTDouble:
					str.dbl = strtod(pReadPos, &pReadPos);
					break;
				}

				if (pReadPos >= m_pMemEnd)
				{
					m_iErr = kErrorEnd;
					return 0;
				}
			}

			str.nLength = (pEndPos ? pEndPos : pReadPos) - pStart;
			if (bQuoteStart)
			{
				str.nLength --;
				pReadPos ++;
			}

			return pReadPos;
		}

		//读取Lua多行字符串
		char* parseFetchLuaString(char* pReadPos, uint32_t eqSyms, sfJSONString& str)
		{
			m_pLastPos = pReadPos;

			char* src = pReadPos;
			size_t i, k, ik, iend = m_pMemEnd - pReadPos - 2;
			for (i = 0; i < iend; ++ i)
			{
				if (src[i] == ']')
				{
					for (k = 1; k < eqSyms; ++ k)
					{
						if (src[i + k] != '=')
							break;
					}

					ik = i + k;
					if (ik < iend && k - 1 == eqSyms && src[ik] == ']')
					{
						str.pString = src;
						str.nLength = i;
						return src + ik;
					}
				}
			}

			m_iErr = kErrorEnd;
			return 0;
		}

	private:
		static inline uint32_t readUnicode(const char* p)
		{
			uint32_t code = 0;
			for (uint32_t i = 0, shift = 12; i < 4; ++ i, shift -= 4)
			{
				uint8_t ch = p[i];
				switch (json_value_char_tbl[ch])
				{
				case 1: code |= (ch - '0') << shift; break;
				case 4: code |= (ch - 'a' + 10) << shift; break;
				case 6: code |= (ch - 'A' + 10) << shift; break;
				}
			}

			return code;
		}

		static inline uint32_t unicode2utf8(uint32_t wchar)
		{
			if (wchar < 0xC0)
				return 1;
			if (wchar < 0x800)
				return 2;
			if (wchar < 0x10000)
				return 3;
			if (wchar < 0x200000)
				return 4;

			return 1;
		}

		static char* unicode2utf8(uint32_t _wchar, char *_utf8)
		{
			char *utf8 = _utf8;
			uint32_t wchar = _wchar, len = 0;

			if (wchar < 0xC0)
			{
				utf8[0] = (char)wchar;
				len = 1;
			}
			else if (wchar < 0x800)
			{
				utf8[0] = 0xc0 | (wchar >> 6);
				utf8[1] = 0x80 | (wchar & 0x3f);
				len = 2;
			}
			else if (wchar < 0x10000)
			{
				utf8[0] = 0xe0 | (wchar >> 12);
				utf8[1] = 0x80 | ((wchar >> 6) & 0x3f);
				utf8[2] = 0x80 | (wchar & 0x3f);
				len = 3;
			}
			else if (wchar < 0x200000)
			{
				utf8[0] = 0xf0 | ((int)wchar >> 18);
				utf8[1] = 0x80 | ((wchar >> 12) & 0x3f);
				utf8[2] = 0x80 | ((wchar >> 6) & 0x3f);
				utf8[3] = 0x80 | (wchar & 0x3f);
				len = 4;
			}

			return utf8 + len;
		}
	};
}

//////////////////////////////////////////////////////////////////////////
class sfNode : public sfNodeBase
{
public:
	typedef void(*fndefNodePrinted)(const char* code, size_t len, void* user);
	static void fnNodePrintToString(const char* code, size_t len, void* user)
	{
		std::string* str = (std::string*)user;
		if (code)
			str->append(code, len);
		else
			str->reserve(len);
	}

public:
	// 按名字顺序查找
	inline sfNode* find(const char* str) const
	{
		sfNode* n = child;
		while (n)
		{
			if (strcmp(n->nameKey, str) == 0)
				return n;
			n = n->next;
		}
		return NULL;
	}
	// 按索引取出，从0开始计数
	inline sfNode* index(uint32_t index) const
	{
		uint32_t i = 0;
		sfNode* n = child;
		while (i < index && n)
			n = n->next;
		return n;
	}
	// 按索引取出，从0开始计数
	inline sfNode* operator [] (uint32_t index) const
	{
		uint32_t i = 0;
		sfNode* n = child;
		while (i < index && n)
			n = n->next;
		return n;
	}

	// 仅创建出一个空节点
	sfNode* createObject(const char* name = NULL, size_t len = 0)
	{
		sfjson::JFile* p = (sfjson::JFile*)pFile;

		sfNode* n = (sfNode*)p->m_nodesPool.allocate();
		memset(n, 0, sizeof(sfNode));

		n->nodeType = JATObject;
		n->pFile = pFile;

		if (name)
			n->name(name, len);

		return n;
	}
	// 仅创建出一个空数组
	sfNode* createArray()
	{
		sfjson::JFile* p = (sfjson::JFile*)pFile;

		sfNode* n = (sfNode*)p->m_nodesPool.allocate();
		memset(n, 0, sizeof(sfNode));

		n->nodeType = JATArray;
		n->pFile = pFile;

		return n;
	}
	// 创建出一个空值
	sfNode* createValue()
	{
		sfjson::JFile* p = (sfjson::JFile*)pFile;

		sfNode* n = (sfNode*)p->m_nodesPool.allocate();
		memset(n, 0, sizeof(sfNode));

		n->pFile = pFile;

		return n;
	}

	// 为本节点创建一个父节点，并将本节点添加进去
	sfNode* createParentObject(const char* name, size_t len = 0)
	{
		sfjson::JFile* p = (sfjson::JFile*)pFile;

		sfNode* n = (sfNode*)p->m_nodesPool.allocate();
		memset(n, 0, sizeof(sfNode));

		n->nodeType = JATObject;
		n->pFile = pFile;
		n->add(this);

		if (name)
			this->name(name, len);

		return n;
	}
	// 创建出一个空数组
	sfNode* createParentArray()
	{
		sfjson::JFile* p = (sfjson::JFile*)pFile;

		sfNode* n = (sfNode*)p->m_nodesPool.allocate();
		memset(n, 0, sizeof(sfNode));

		n->nodeType = JATArray;
		n->pFile = pFile;
		n->add(this);

		return n;
	}

	// 将一个已经创建好的节点添加到本节点下成为子节点并返回this
	sfNode* add(sfNode* n)
	{
		childCount ++;
		if (child)
			childLast->next = n;
		else
			child = n;
		childLast = n;

		return this;
	}
	// 将一个已经创建好的节点添加到本节点下成为子节点并在设置名字后返回this
	sfNode* add(sfNode* n, const char* name, size_t len = 0)
	{
		n->name(name, len);

		childCount ++;
		if (child)
			childLast->next = n;
		else
			child = n;
		childLast = n;

		return this;
	}

	// 设置节点的名字
	sfNode* name(const char* name, size_t len = 0)
	{
		if (name)
		{
			sfjson::JFile* p = (sfjson::JFile*)pFile;
			nameLength = len ? len : strlen(name);
			nameKey = p->m_strBuf.addString(name, nameLength + 1);
			nameKey[nameLength] = 0;
		}

		return this;
	}

	// 设置本节点的值为布尔型
	sfNode* val(bool val)
	{
		assert(nodeType < JATObject);

		if (nodeType < JATObject)
		{
			bval = val;
			valLength = -1;
			nodeType = JATBooleanValue;
		}
		return this;
	}
	// 设置本节点的值为整数型
	sfNode* val(int64_t val)
	{
		assert(nodeType < JATObject);

		if (nodeType < JATObject)
		{
			ival = val;
			valLength = -1;
			nodeType = JATIntValue;
		}
		return this;
	}
	sfNode* val(int val)
	{
		assert(nodeType < JATObject);

		if (nodeType < JATObject)
		{
			ival = val;
			valLength = -1;
			nodeType = JATIntValue;
		}
		return this;
	}
	// 设置本节点的值为小数型
	sfNode* val(double val)
	{
		assert(nodeType < JATObject);

		if (nodeType < JATObject)
		{
			dval = val;
			valLength = -1;
			nodeType = JATDoubleValue;
		}
		return this;
	}
	// 设置本节点的值为字符串型。如果不带参数调用则值设置为null节点
	sfNode* val(const char* val = NULL, size_t len = 0)
	{
		assert(nodeType < JATObject);

		if (nodeType < JATObject)
		{
			sfjson::JFile* p = (sfjson::JFile*)pFile;
			if (val)
			{
				nodeType = JATString;
				valLength = len ? len : strlen(val);
				strval = p->m_strBuf.addString(val, valLength + 1);
				strval[valLength] = 0;
			}
			else
			{
				strval = NULL;
				valLength = -1;
				nodeType = JATNullValue;
			}
		}
		return this;
	}

	// 整数、布尔、小数转浮点型，其它类型返回0
	inline double toDouble() const
	{
		if (nodeType == JATDoubleValue)
			return dval;
		if (nodeType == JATIntValue || nodeType == JATBooleanValue)
			return ival;
		return 0;
	}
	// 整数、布尔、小数转整数型，其它类型返回0
	inline int64_t toInteger() const
	{
#define D2I_ENDIALOC	0

		if (nodeType == JATIntValue || nodeType == JATBooleanValue)
			return ival;
		if (nodeType == JATDoubleValue)
		{
			union d2int
			{
				double	dval;
				int32_t	i32[2];
			} u;

			u.dval = dval + 6755399441055744.0;
			return u.i32[D2I_ENDIALOC];
		}
		return 0;
	}
	// 转字符串
	void toString(std::string& strOut) const
	{
		size_t len;
		char szBuf[32];

		switch (nodeType)
		{
		case JATString:
			strOut.append(strval, valLength);
			break;

		case JATBooleanValue:
			if (bval)
				strOut.append("true", 4);
			else
				strOut.append("false", 5);
			break;

		case JATIntValue:
#ifdef _SFJSON_CRTOPT_H__
			len = opt_i64toa(ival, szBuf);
#else
			len = sprintf(szBuf, "%lld", ival);
#endif
			strOut.append(szBuf, len);
			break;

		case JATDoubleValue:
#ifdef _SFJSON_CRTOPT_H__
			len = opt_dtoa(dval, szBuf);
#else
			len = sprintf(szBuf, "%f", dval);
#endif
			strOut.append(szBuf, len);
			break;

		case JATNullValue:
			strOut.append("null", 4);
			break;
		}
	}

	// 直接获取小数
	template <typename R> R toInt() const
	{
		return (R)ival;
	}

	// 格式化输出为字符串
	inline bool printTo(std::string& strOut, uint32_t flags = kJsonUnicodes)
	{
		return printTo(fnNodePrintToString, &strOut, flags);
	}

	bool printTo(fndefNodePrinted fn, void* user, uint32_t flags = kJsonUnicodes)
	{
		char fixbuf[SFJ_ENCODE_INIT_MEMSIZE];
		sfjson::JFile* p = (sfjson::JFile*)pFile;
		sfjson::MemList encodeBuf;
		char beginChar;

		switch (nodeType)
		{
		case JATObject: beginChar = '{'; break;
		case JATArray: beginChar = '['; break;
		default: return false;
		}

		encodeBuf.wrapNode(fixbuf, sizeof(fixbuf));

		sfNode* node = child;
		while (node)
		{
			if (node != child)
				encodeBuf.addChar(',');
			p->printToBuffer(encodeBuf, node, this, flags);
			node = node->next;
		}

		// 统计
		size_t total = 0;
		sfjson::MemNode* n = encodeBuf.first;

		while (n)
		{
			total += n->used;
			n = n->next;
		}

		if (fn)
		{
			fn(NULL, total + 2, user);
			fn(&beginChar, 1, user);

			// 合并
			n = encodeBuf.first;
			while (n)
			{
				fn((char*)(n + 1), n->used, user);
				n = n->next;
			}

			beginChar = beginChar == '{' ? '}' : ']';
			fn(&beginChar, 1, user);
		}

		return true;
	}

	// 销毁
	void destroy()
	{
		if (pFile)
		{
			sfjson::JFile* p = (sfjson::JFile*)pFile;
			p->~JFile();
			free(p);
		}
	}

public:// 链式操作函数组
	   // 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* append(bool val)
	{
		addValue()->val(val);
		return this;
	}
	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* append(int64_t val)
	{
		addValue()->val(val);
		return this;
	}
	inline sfNode* append(int val)
	{
		addValue()->val(val);
		return this;
	}
	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* append(double val)
	{
		addValue()->val(val);
		return this;
	}
	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* append(const char* val = NULL, size_t len = 0)
	{
		addValue()->val(val, len);
		return this;
	}

	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* appendNamed(const char* name, bool val)
	{
		addValue()->name(name)->val(val);
		return this;
	}
	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* appendNamed(const char* name, int64_t val)
	{
		addValue()->name(name)->val(val);
		return this;
	}
	inline sfNode* appendNamed(const char* name, int val)
	{
		addValue()->name(name)->val(val);
		return this;
	}
	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* appendNamed(const char* name, double val)
	{
		addValue()->name(name)->val(val);
		return this;
	}
	// 创建值型节点同时添加为子节点并返回本节点（非刚创建的子节点）
	inline sfNode* appendNamed(const char* name, const char* val = NULL, size_t len = 0)
	{
		addValue()->name(name)->val(val, len);
		return this;
	}

	// 创建各种值类型的节点
	inline sfNode* createValue(bool val)
	{
		return createValue()->val(val);
	}
	inline sfNode* createValue(int64_t val)
	{
		return createValue()->val(val);
	}
	inline sfNode* createValue(double val)
	{
		return createValue()->val(val);
	}
	inline sfNode* createValue(const char* val, size_t len = 0)
	{
		return createValue()->val(val, len);
	}

	// 创建节点同时添加为子节点并返回刚创建的节点
	inline sfNode* addObject(const char* name = NULL, size_t len = 0)
	{
		sfNode* n = createObject(name, len); 
		add(n);
		return n;
	}
	// 创建数组同时添加为子节点并返回刚创建的节点
	inline sfNode* addArray(const char* name = NULL, size_t len = 0)
	{
		sfNode* n = createArray(); 
		if (name) n->name(name, len);
		add(n);
		return n;
	}

	// 创建值同时添加为子节点并返回刚创建的节点
	inline sfNode* addValue() { sfNode* n = createValue(); add(n); return n; }

	// 创建值同时添加为子节点并根据值类型设置完值后，返回刚创建的节点
	inline sfNode* addValue(bool val) { sfNode* n = createValue()->val(val); add(n); return n; }
	inline sfNode* addValue(int64_t val) { sfNode* n = createValue()->val(val); add(n); return n; }
	inline sfNode* addValue(double val) { sfNode* n = createValue()->val(val); add(n); return n; }
	inline sfNode* addValue(const char* val, size_t len = 0) { sfNode* n = createValue()->val(val, len); add(n); return n; }
};

//////////////////////////////////////////////////////////////////////////
// 从Json代码解析
static sfNode* sfJsonDecode(const char* code, size_t len, bool bCopyCode = false)
{
	size_t s = sizeof(sfjson::JFile);
	if (bCopyCode)
		s += len;

	sfjson::JFile* p = (sfjson::JFile*)malloc(s);
	new (p) sfjson::JFile(len);

	if (bCopyCode)
	{
		memcpy((char*)(p + 1), code, len);
		if (p->parse(NULL) > 0)
			return p->getRoot();
	}
	else if (p->parse(const_cast<char*>(code)) > 0)
	{
		return p->getRoot();
	}

	p->~JFile();
	free(p);

	return NULL;
}
static inline sfNode* sfJsonDecode(const std::string& strInput, bool bCopyCode = false)
{
	return sfJsonDecode(strInput.c_str(), strInput.length(), bCopyCode);
}

// 直接使用静态地址，同时不再分配内存也不再复制Json字符串
#define sfJsonDecl(varname) char varname[sizeof(sfjson::JFile)]

static inline sfNode* sfJsonDecodeFixed(void* jFileMem, const char* code, size_t len)
{
	sfjson::JFile* p = (sfjson::JFile*)jFileMem;
	new (p) sfjson::JFile(len);

	if (p->parse(const_cast<char*>(code)) > 0)
		return p->getRoot();

	return NULL;
}
static sfNode* sfJsonDecodeFixed(void* jFileMem, const std::string& strInput)
{
	return sfJsonDecodeFixed(jFileMem, strInput.c_str(), strInput.length());
}

//////////////////////////////////////////////////////////////////////////
// 创建根节点用于编码
static sfNode* sfJsonCreate(bool bIsObject, size_t initMemSize = SFJ_ENCODE_INIT_MEMSIZE)
{
	sfjson::JFile* p = (sfjson::JFile*)malloc(std::max(256, SFJ_ENCODE_INIT_MEMSIZE) + sizeof(sfjson::JFile));
	new (p) sfjson::JFile((char*)(p + 1), std::max(256, SFJ_ENCODE_INIT_MEMSIZE));

	return p->newRoot(bIsObject);
}

#endif