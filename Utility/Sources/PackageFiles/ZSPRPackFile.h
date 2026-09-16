#ifndef ZSPRPACKFILE_H
#define ZSPRPACKFILE_H

#include "ZPackFile.h"

typedef struct {
	long compress_size;
	long size;
} frame_info;

typedef struct {
	unsigned long	id;
	int				frame;
} frame_index_info;

#define MAX_IMAGE		0x10													//最多临时文件的个数

class ZSPRPackFile : public ZPackFile {
public:
	ZSPRPackFile(const char *file_name, ZCache *the_cache);
	~ZSPRPackFile();
	char *getSPR(const char *name, int frame, char **frame_data, bool b3D);				//获取指定帧的数据
//	char *get3DSPR(const char *name, int frame, char **frame_data);				//获取三维的帧数据
};

void make_palette(unsigned char *palette, int count);
#endif