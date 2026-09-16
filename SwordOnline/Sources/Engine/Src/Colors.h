#pragma once
#ifndef Colors_H
#define Colors_H
#include <math.h>
#include <cmath>       // parse the CRT's own fmaxf/fminf before they are shadowed by these macros
#define fmaxf(a,b)            (((a) > (b)) ? (a) : (b))
#define fminf(a,b)            (((a) < (b)) ? (a) : (b))
enum COLORS_BLEND_MODE
{
    /*0 */	CBM_MULTIPLY = 0,
	/*1 */	CBM_HUE,
	/*2 */	CBM_COLOR,
	/*3 */	CBM_SATURATION,
	/*4 */	CBM_LUMINOSITY,
	/*5 */	CBM_SCREEN,
	/*6 */	CBM_OVERLAY,
	/*7 */	CBM_DARKEN,
	/*8 */	CBM_LIGHTEN,
	/*9 */	CBM_COLORDODGE,
	/*10*/	CBM_COLORBURN,
	/*11*/	CBM_HARDLIGHT,
	/*12*/	CBM_SOFTLIGHT,
	/*13*/	CBM_DIFFERENCE,
	/*14*/	CBM_EXCLUSION,
	/*15*/	CBM_COUNT,
};

inline float gClrMin(float R, float G, float B)
{
	float fC = R;
	if(fC > G)
		fC = G;
	if(fC > B)
		fC = B;
	return fC;
}

inline float gClrMax(float R, float G, float B)
{
	float fC = R;
	if(fC < G)
		fC = G;
	if(fC < B)
		fC = B;
	return fC;
}

inline float gLum(float R, float G, float B)
{
	return (R*0.3f + G*0.59f + B*0.11f);
}

inline float gSat(float R, float G, float B)
{
	return (gClrMax(R,G,B) - gClrMin(R,G,B));
}

inline void gClipColor(float& R, float& G, float& B)
{
	float fL = gLum(R,G,B);
	float fMin = gClrMin(R,G,B);
	float fMax = gClrMax(R,G,B);
	if(fMin < 0.f)
	{
		R = fL + ((R-fL)*fL)/(fL-fMin);
		G = fL + ((G-fL)*fL)/(fL-fMin);
		B = fL + ((B-fL)*fL)/(fL-fMin);
	}
	if(fMax > 1.f)
	{
		R = fL + ((R-fL)*(1.f-fL))/(fMax-fL);
		G = fL + ((G-fL)*(1.f-fL))/(fMax-fL);
		B = fL + ((B-fL)*(1.f-fL))/(fMax-fL);
	}
}

inline void gSetLum(float& R, float& G, float& B, float fL)
{
	float d = fL - gLum(R,G,B);
	R += d;
	G += d;
	B += d;
	gClipColor(R,G,B);
}

inline void gSetSat(float& R, float& G, float& B, float fS)
{
	int nMin = 0, nMid = 0, nMax = 0;
	float fMin = R;
	if(fMin > G)
	{
		nMin = 1;
		fMin = G;
		if(fMin > B)
		{
			nMin = 2;
			fMin = B;
		}
	}
	float fMax = R;
	if(fMax <= G)
	{
		nMax = 1;
		fMax = G;
		if(fMax <= B)
		{
			nMax = 2;
			fMax = B;
		}
	}
	float fMid = R;
	if(nMid == nMin || nMid == nMax)
	{
		nMid++;
		fMid = G;
		if(nMid == nMin || nMid == nMax)
		{
			nMid++;
			fMid = B;
		}
	}
	if(fMax > fMin)
	{
		fMid = ((fMid-fMin)*fS)/(fMax-fMin);
		fMax = fS;
	}
	else
		fMid = fMax = 0.f;
	fMin = 0.f;
	if(nMin == 0)
		R = fMin;
	else if(nMin == 1)
		G = fMin;
	else
		B = fMin;
	if(nMid == 0)
		R = fMid;
	else if(nMid == 1)
		G = fMid;
	else
		B = fMid;
	if(nMax == 0)
		R = fMax;
	else if(nMax == 1)
		G = fMax;
	else
		B = fMax;
}

inline void gBlendMode_Color(float& R, float& G, float& B, float bR, float bG, float bB)
{
	gSetLum(R,G,B,gLum(bR,bG,bB));
}

inline void gBlendMode_Hue(float& R, float& G, float& B, float bR, float bG, float bB)
{
	gSetSat(R,G,B,gSat(bR,bG,bB));
	gSetLum(R,G,B,gLum(bR,bG,bB));
}

inline void gBlendMode_Sat(float& R, float& G, float& B, float bR, float bG, float bB)
{
	float dR = bR,dG = bG, dB = bB;
	gSetSat(dR,dG,dB,gSat(R,G,B));
	gSetLum(dR,dG,dB,gLum(bR,bG,bB));
	R = dR;
	G = dG;
	B = dB;
}

inline void gBlendMode_Lum(float& R, float& G, float& B, float bR, float bG, float bB)
{
	float dR = bR,dG = bG, dB = bB;
	gSetLum(dR,dG,dB,gLum(R,G,B));
	R = dR;
	G = dG;
	B = dB;
}

inline void gBlendMode_Screen(float& R, float& G, float& B, float bR, float bG, float bB)
{
	R = R + bR - R*bR;
	G = G + bG - G*bG;
	B = B + bB - B*bB;
}

inline void gBlendMode_HardLight(float& R, float& G, float& B, float bR, float bG, float bB)
{
	if(R <= 0.5f)
		R = R*2.f*bR;
	else
	{
		R = (R*2.f-1.f);
		R = R + bR - R*bR;
	}
	if(G <= 0.5f)
		G = G*2.f*bG;
	else
	{
		G = (G*2.f-1.f);
		G = G + bG - G*bG;
	}
	if(B <= 0.5f)
		B = B*2.f*bB;
	else
	{
		B = (B*2.f-1.f);
		B = B + bB - B*bB;
	}
}

inline void gBlendMode_OverLay(float& R, float& G, float& B, float bR, float bG, float bB)
{
	float dR = bR,dG = bG, dB = bB;
	gBlendMode_HardLight(dR,dG,dB, R,G,B);
	R = dR;
	G = dG;
	B = dB;
}

inline void gBlendMode_Darken(float& R, float& G, float& B, float bR, float bG, float bB)
{
	R = fminf(R,bR);
	G = fminf(G,bG);
	B = fminf(B,bB);
}

inline void gBlendMode_Lighten(float& R, float& G, float& B, float bR, float bG, float bB)
{
	R = fmaxf(R,bR);
	G = fmaxf(G,bG);
	B = fmaxf(B,bB);
}

inline void gBlendMode_ColorDodge(float& R, float& G, float& B, float bR, float bG, float bB)
{
	if(R < 1.f)
		R = fminf(1.f,bR/(1.f-R));
	else
		R = 1.f;
	if(G < 1.f)
		G = fminf(1.f,bG/(1.f-G));
	else
		G = 1.f;
	if(B < 1.f)
		B = fminf(1.f,bB/(1.f-B));
	else
		B = 1.f;
}

inline void gBlendMode_ColorBurn(float& R, float& G, float& B, float bR, float bG, float bB)
{
	if(R > 0.f)
		R = 1.f-fminf(1.f,(1.f-bR)/R);
	else
		R = 0.f;
	if(G > 0.f)
		G = 1.f-fminf(1.f,(1.f-bG)/G);
	else
		G = 0.f;
	if(B > 0.f)
		B = 1.f-fminf(1.f,(1.f-bB)/B);
	else
		B = 0.f;
}

inline float gSoftLightD(float c)
{
	float r;
	if(c <= 0.25f)
		r = ((16.f*c-12.f)*c+4.f)*c;
	else
		r = sqrtf(c);
	return r;
}

inline void gBlendMode_SoftLight(float& R, float& G, float& B, float bR, float bG, float bB)
{
	if(R <= 0.5f)
		R = bR-(1.f-2.f*R)*bR*(1.f-bR);
	else
		R = bR+(2.f*R-1.f)*(gSoftLightD(bR)-bR);
	if(G <= 0.5f)
		G = bG-(1.f-2.f*G)*bG*(1.f-bG);
	else
		G = bG+(2.f*G-1.f)*(gSoftLightD(bG)-bG);
	if(B <= 0.5f)
		B = bB-(1.f-2.f*B)*bB*(1.f-bB);
	else
		B = bB+(2.f*B-1.f)*(gSoftLightD(bB)-bB);
}

inline void gBlendMode_Difference(float& R, float& G, float& B, float bR, float bG, float bB)
{
	R = fabsf(bR-R);
	G = fabsf(bG-G);
	B = fabsf(bB-B);
}

inline void gBlendMode_Exclusion(float& R, float& G, float& B, float bR, float bG, float bB)
{
	R = bR+R-2.f*bR*R;
	G = bG+G-2.f*bG*G;
	B = bB+B-2.f*bB*B;
}

#endif
