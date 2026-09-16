
#include"stdAfx.h"
#include "KWin32.h"
#include "ZSPRPackFile.h"
#include "ucl/ucl.h"

typedef struct {
	BYTE	Comment[4];	// 注释文字(SPR\0)
	WORD	Width;		// 图片宽度
	WORD	Height;		// 图片高度
	WORD	CenterX;	// 重心的水平位移
	WORD	CenterY;	// 重心的垂直位移
	WORD	Frames;		// 总帧数
	WORD	Colors;		// 颜色数
	WORD	Directions;	// 方向数
	WORD	Interval;	// 每帧间隔（以游戏帧为单位）
	WORD	Reserved[6];// 保留字段（到以后使用）
} SPRHEAD;

typedef struct
{
	DWORD	Offset;		// 每一帧的偏移
	DWORD	Length;		// 每一帧的长度
} SPROFFS;


ZSPRPackFile::ZSPRPackFile(const char *file_name, ZCache *the_cache) : ZPackFile(file_name, the_cache) {
}

ZSPRPackFile::~ZSPRPackFile() {
}

void make_palette(unsigned char *palette, int count) {
	int index = 0;
	unsigned char *source = palette;
	unsigned short color;
	unsigned short *dest = (unsigned short *)palette;
	for(index = 0; index < count; index++) {
		color = *source++;
		color >>= 3;
		color <<= 11;
		*dest = color;
		color = *source++;
		color >>= 2;
		color <<= 5;
		*dest |= color;
		color = *source++;
		color >>= 3;
		*dest |= color;
		dest++;
	}
}

char *ZSPRPackFile::getSPR(const char *name, int frame, char **frame_data, bool b3D)
{					//获取指定的spr的指定帧数据
	SPRHEAD *header;
	char *node;
	unsigned long id = hash(name);
	int node_index = getNodeIndex(id);
	if(node_index == -1) return 0;
//首先检查这个id是什么类型压缩方式
	unsigned long compress_type = index_list[node_index].compress_size >> 24;
	if((compress_type & TYPE_FRAME) == 0) {						//没有采用帧的独立压缩，还是需要进行缓存
		node = cache->searchNode((const char *)&id, sizeof(id));
		if(!node) {
			node = cache->getNode(sizeof(unsigned long), index_list[node_index].size);
			if(!node) return 0;
			*(unsigned long *)node = id;
			node += sizeof(unsigned long);
			if(!_readData(node_index, node)) return 0;
//需要进行调色板的转换24->16(555或者4444调色板)
			if(!b3D) make_palette((unsigned char *)node + sizeof(SPRHEAD), ((SPRHEAD *)node)->Colors);
		}
		if(!node) return 0;
//找到其中的一帧数据
		header = (SPRHEAD *)node;
		if(frame >= header->Frames) return 0;
		SPROFFS *frames = (SPROFFS *)(node + sizeof(SPRHEAD) + header->Colors * 3);
		if(frame_data) *frame_data = (char *)frames + header->Frames * sizeof(SPROFFS) + frames[frame].Offset;
		return node;
	}
	compress_type &= 0x0F;
//采用了帧独立压缩，首先取出没有压缩的头信息						
//头信息需要把调色板的信息取出来，进行24->16位的色彩转换，所以需要缓存
	char *map_ptr = data_map->map(index_list[node_index].offset, MAX_BLOCK_SIZE);
	if(!map_ptr) return 0;									//失败
	char *source = cache->searchNode((const char *)&id, sizeof(id));
	if(!source) {												//缓冲区中没有头信息
		header = (SPRHEAD *)map_ptr;
		if(frame >= header->Frames) return 0;
		int block_size = sizeof(SPRHEAD) + header->Colors * 3 + header->Frames * sizeof(SPROFFS);
		source = cache->getNode(sizeof(unsigned long), block_size);
		if(!source) return 0;
		*(unsigned long *)source = id;
		source += sizeof(unsigned long);
		memmove(source, map_ptr, block_size);
		if(!b3D) make_palette((unsigned char *)source + sizeof(SPRHEAD), header->Colors);
	}
	else header = (SPRHEAD *)source;
	if(frame < 0) return source;							//仅仅取得头信息

	frame_index_info frame_index;
	frame_index.id = id;
	frame_index.frame = frame;

	*frame_data = cache->searchNode((char *)&frame_index, sizeof(frame_index));
	if(*frame_data) return source;								//在缓冲区中找到了！
//读出指定帧的信息
	frame_info *frame_list = (frame_info *)(source + sizeof(SPRHEAD) + header->Colors * 3);
	char *source_frame = (char *)map_ptr + sizeof(SPRHEAD) + header->Colors * 3 + header->Frames * sizeof(frame_info);
	while(frame--) source_frame += (frame_list++)->compress_size;
	int frame_size = (int)frame_list->size;
	if(frame_size < 0) {
		node = cache->getNode(sizeof(unsigned long) + sizeof(unsigned long), -frame_size);
		memmove(node + sizeof(unsigned long) + sizeof(unsigned long), source_frame, -frame_size);
	}
	else {
		node = cache->getNode(sizeof(unsigned long) + sizeof(unsigned long), frame_size);
		int r;
		unsigned int dest_length = frame_list->size;
		if(compress_type == TYPE_UCL) {
			r = ucl_nrv2b_decompress_8((BYTE *)source_frame, frame_list->compress_size, (BYTE *)node + sizeof(unsigned long) + sizeof(unsigned long), &dest_length, NULL);
		}
		else if(compress_type == TYPE_BZIP2) {
//			r = BZ2_bzBuffToBuffDecompress(node + sizeof(unsigned long) + sizeof(unsigned long), &dest_length, source_frame, frame_list->compress_size, 0, 0);
		}
	}
	*(unsigned long *)node = frame_index.id;
	*(unsigned long *)(node + sizeof(unsigned long)) = frame_index.frame;
	*frame_data = node + sizeof(unsigned long) + sizeof(unsigned long);
	return source;
}
