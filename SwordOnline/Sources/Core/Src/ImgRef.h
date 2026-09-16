#pragma once

extern struct iRepresentShell*	g_pRepresent;

void		 IR_CUpdateTime();
unsigned int IR_CGetCurrentTime();
void		 IR_CNextFrame(int& nFrame, int nTotalFrame, unsigned int uInterval, unsigned int& uFlipTime);