//=======================================
//		绘制sprite用到的汇编程序片断
//=======================================

#define		read_alpha_2_ebx_run_length_2_eax		    \
				{	movzx	eax, byte ptr[esi]	}	    \
				{	movzx	ebx, byte ptr[esi + 1]	}	    \
				{	add		esi, 2   			}


#define		copy_pixel_use_eax	/*ebx指向调色板*/	\
				{	xor	    eax, eax            }	\
				{	mov	    al, byte ptr [esi]	}	\
				{	add		esi, 1 				}	\
				{	mov		ax, [ebx + eax * 2]	}	\
				{	add		edi, 2				}   \
				{	mov		[edi - 2], ax			}

#define		copy_4pixel_use_eax	/*ebx指向调色板*/	\
				{	movd    mm4, [esi]	        } 	\
				{	add		esi, 4 				}	\
                {   movd    eax, mm4            }   \
                {   and     eax, 0xff           }   \
				{	mov		ax, [ebx + eax * 2]	}	\
				{	add		edi, 2				}   \
				{	mov		[edi - 2], ax		}   \
                {   movd    eax, mm4            }   \
                {   shr     eax, 8              }   \
                {   and     eax, 0xff           }   \
				{	mov		ax, [ebx + eax * 2]	}	\
				{	add		edi, 2				}   \
				{	mov		[edi - 2], ax		}   \
                {   movd    eax, mm4            }   \
                {   shr     eax, 16             }   \
                {   and     eax, 0xff           }   \
				{	mov		ax, [ebx + eax * 2]	}	\
				{	add		edi, 2				}   \
				{	mov		[edi - 2], ax		}   \
                {   movd    eax, mm4            }   \
                {   shr     eax, 24             }   \
                {   and     eax, 0xff           }   \
				{	mov		ax, [ebx + eax * 2]	}	\
				{	add		edi, 2				}   \
				{	mov		[edi - 2], ax		}

#define		copy_pixel_use_eax_32b	/*ebx palette 3 colors*/	\
				{	movzx   eax, byte ptr [esi]	}	\
				{	imul	eax, 3            	}   \
				{	movd	mm4, eax            }   \
				{	inc		esi					}	\
				{	push	ecx                 }   \
				{	push	edx                 }   \
				{	movzx	eax, byte ptr[ebx + eax]	}	\
				{	shl		eax, 16             }   \
				{	push	eax                 }   \
				{	movd	eax, mm4            }   \
				{	movzx	ecx, byte ptr[ebx + eax + 1]	}\
				{	shl		ecx, 8              }   \
				{	movzx	edx, byte ptr[ebx + eax + 2]	}\
				{	pop		eax                 }   \
				{	or		eax,ecx             }   \
				{	or		eax,edx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}   \
				{	pop		edx                 }   \
				{	pop		ecx                 }   

#define		copy_4pixel_use_eax_32b	/*ebx palette 3 colors*/	\
				{	movd    mm4, [esi]	        } 	\
				{	add		esi, 4 				}	\
				{	push	ecx                 }   \
				{	push	edx                 }   \
                {   movd    eax, mm4            }   \
                {   and     eax, 0xff           }   \
                {   imul    eax, 3           	}   \
				{	movd	mm7, eax            }   \
				{	movzx	eax, byte ptr[ebx + eax]	}\
				{	shl		eax, 16             }   \
				{	push	eax                 }   \
				{	movd	eax, mm7            }   \
				{	movzx	ecx, byte ptr[ebx + eax + 1]	}\
				{	shl		ecx, 8              }   \
				{	movzx	edx, byte ptr[ebx + eax + 2]	}\
				{	pop		eax                 }   \
				{	or		eax,ecx             }   \
				{	or		eax,edx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}   \
                {   movd    eax, mm4            }   \
                {   shr     eax, 8              }   \
                {   and     eax, 0xff           }   \
                {   imul    eax, 3           	}   \
				{	movd	mm7, eax            }   \
				{	movzx	eax, byte ptr[ebx + eax]	}\
				{	shl		eax, 16             }   \
				{	push	eax                 }   \
				{	movd	eax, mm7            }   \
				{	movzx	ecx, byte ptr[ebx + eax + 1]	}\
				{	shl		ecx, 8              }   \
				{	movzx	edx, byte ptr[ebx + eax + 2]	}\
				{	pop		eax                 }   \
				{	or		eax,ecx             }   \
				{	or		eax,edx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}   \
                {   movd    eax, mm4            }   \
                {   shr     eax, 16             }   \
                {   and     eax, 0xff           }   \
                {   imul    eax, 3           	}   \
				{	movd	mm7, eax            }   \
				{	movzx	eax, byte ptr[ebx + eax]	}\
				{	shl		eax, 16             }   \
				{	push	eax                 }   \
				{	movd	eax, mm7            }   \
				{	movzx	ecx, byte ptr[ebx + eax + 1]	}\
				{	shl		ecx, 8              }   \
				{	movzx	edx, byte ptr[ebx + eax + 2]	}\
				{	pop		eax                 }   \
				{	or		eax,ecx             }   \
				{	or		eax,edx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}   \
                {   movd    eax, mm4            }   \
                {   shr     eax, 24             }   \
                {   and     eax, 0xff           }   \
                {   imul    eax, 3           	}   \
				{	movd	mm7, eax            }   \
				{	movzx	eax, byte ptr[ebx + eax]	}\
				{	shl		eax, 16             }   \
				{	push	eax                 }   \
				{	movd	eax, mm7            }   \
				{	movzx	ecx, byte ptr[ebx + eax + 1]	}\
				{	shl		ecx, 8              }   \
				{	movzx	edx, byte ptr[ebx + eax + 2]	}\
				{	pop		eax                 }   \
				{	or		eax,ecx             }   \
				{	or		eax,edx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}   \
				{	mov		[edi - 4], eax		}   \
				{	pop		edx                 }   \
				{	pop		ecx                 }   
				
#define		mix_2_pixel_color_alpha_use_eabdx_32b						\
				{	movd	mm7, ecx    		}						\
				{	movzx   eax, byte ptr [esi]	}	\
				{	imul	eax, 3            	}   \
				{	movd	mm4, eax            }   \
				{	inc		esi					}						\
				{	movd	edx, mm3			}	/*	nAlpha    */	\
				{	movd    ebx, mm0    		}	/* pPalette */		\
				{	movzx	eax, byte ptr[ebx + eax]	}	\
				{	imul	eax, edx            }   /*nAlpha*color*/\
				{	mov		ebx, [edi]			}	/*bgcolor*/\
				{	shr		ebx, 16				}	\
				{	and		ebx, 0xff			}	\
				{	mov		ecx, 255			}	\
				{	sub		ecx, edx			}	/*(255-nAlpha)*/\
				{	imul	ecx, ebx            }   /*(255-nAlpha)*color*/\
				{	add		eax, ecx			}	/*nAlpha*c1+(255-nAlpha)*c2*/\
				{	shr		eax, 8				}   /*(nAlpha*c1+(255-nAlpha)*c2)/255*/\
				{	shl		eax, 16				}	\
				{	push	eax                 }   \
				{	movd	eax, mm4            }   \
				{	movd    ebx, mm0    		}	/* pPalette */		\
				{	movzx	eax, byte ptr[ebx + eax + 1]	}	\
				{	imul	eax, edx            }   /*nAlpha*color*/\
				{	mov		ebx, [edi]			}	/*bgcolor*/\
				{	shr		ebx, 8				}	\
				{	and		ebx, 0xff			}	\
				{	mov		ecx, 255			}	\
				{	sub		ecx, edx			}	/*(255-nAlpha)*/\
				{	imul	ecx, ebx            }   /*(255-nAlpha)*color*/\
				{	add		eax, ecx			}	/*nAlpha*c1+(255-nAlpha)*c2*/\
				{	shr		eax, 8				}   /*(nAlpha*c1+(255-nAlpha)*c2)/255*/\
				{	shl		eax, 8				}	\
				{	push	eax                 }   \
				{	movd	eax, mm4            }   \
				{	movd    ebx, mm0    		}	/* pPalette */		\
				{	movzx	eax, byte ptr[ebx + eax + 2]	}	\
				{	imul	eax, edx            }   /*nAlpha*color*/\
				{	mov		ebx, [edi]			}	/*bgcolor*/\
				{	and		ebx, 0xff			}	\
				{	mov		ecx, 255			}	\
				{	sub		ecx, edx			}	/*(255-nAlpha)*/\
				{	imul	ecx, ebx            }   /*(255-nAlpha)*color*/\
				{	add		eax, ecx			}	/*nAlpha*c1+(255-nAlpha)*c2*/\
				{	shr		eax, 8				}   /*(nAlpha*c1+(255-nAlpha)*c2)/255*/\
				{	pop		ebx                 }   \
				{	or		eax,ebx             }   \
				{	pop		ebx                 }   \
				{	or		eax,ebx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}	\
				{	mov		[edi-4], eax		}	\
				{	movd    ecx, mm7    		}	

#define		mix_2_pixel_color_use_eabdx									\
				{	movd	mm7, ecx			}						\
                {   xor     eax, eax            }                       \
				{	movd    ebx, mm0    		}	/* pPalette */		\
				{	mov	    al, byte ptr[esi]	}						\
				{	inc		esi					}						\
				{	mov     dx, [ebx + eax * 2]	}	/*edx = ...rgb*/	\
				{	movd	ecx, mm2    		}	/* nMask32 */		\
				{	mov		ax, dx				}	/*eax = ...rgb*/	\
				{	shl		eax, 16				}	/*eax = rgb...*/	\
				{	mov		ax, dx				}	/*eax = rgbrgb*/	\
				{	and		eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
				{	mov		bx, dx				}	/*ebx = ...rgb*/	\
				{	shl		ebx, 16				}	/*ebx = rgb...*/	\
				{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
				{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
                {   lea     edx, [ebx + ebx * 2]}                       \
                {   add     eax, edx            }                       \
				{	shr		eax, 2				}	/*c = (3xc1+c2)/4*/	\
				{	and     eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov     dx, ax				}	/*edx = ...r.b*/	\
				{	shr     eax, 16				}	/*eax = ....g.*/	\
				{	add 	edi, 2				}						\
				{	or      ax, dx				}	/*eax = ...rgb*/	\
				{	movd     ecx, mm7			}                       \
				{	mov		[edi - 2], ax		}

#define		mix_2_pixel_color_alpha_use_eabdx							\
				{	movd	mm7, ecx    		}						\
				{	xor	    eax, eax    		}						\
				{	movd    ebx, mm0    		}	/* pPalette */		\
				{	mov  	al, byte ptr[esi]	}						\
				{	inc		esi					}						\
				{	mov     dx, [ebx + eax * 2]	}	/*edx = ...rgb*/	\
				{	movd	ecx, mm2    		}   /* nMask32 */		\
				{	mov		ax, dx				}	/*eax = ...rgb*/	\
				{	shl		eax, 16				}	/*eax = rgb...*/	\
				{	mov		ax, dx				}	/*eax = rgbrgb*/	\
				{	and		eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
				{	mov		bx, dx				}	/*ebx = ...rgb*/	\
				{	shl		ebx, 16				}	/*ebx = rgb...*/	\
				{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
				{	movd	edx, mm3			}	/*	nAlpha    */	\
				{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
                {   sub     eax, ebx            }   /*eax = c1 - c2*/   \
				{	imul    eax, edx			}	/*eax = (c1 - c2)*nAlpha */	\
				{	shr		eax, 5				}	/*c=(c1 - c2)*nAlpha/32*/ \
                {   add     eax, ebx            }   /*c=(c1 - c2)*nAlpha/32 + c2*/ \
				{	and     eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov     dx, ax				}	/*edx = ...r.b*/	\
				{	shr     eax, 16				}	/*eax = ....g.*/	\
				{	add		edi, 2				}						\
				{	or      ax, dx				}	/*eax = ...rgb*/	\
				{	movd    ecx, mm7    		}                       \
				{	mov		[edi - 2], ax		}

#define		mix_2_pixel_color_alpha_onrec_16	\
				{	mov     dx, cx				}	/*edx = ...rgb*/	\
				{	movd	ecx, mm2    		}   /* nMask32 */		\
				{	mov		ax, dx				}	/*eax = ...rgb*/	\
				{	shl		eax, 16				}	/*eax = rgb...*/	\
				{	mov		ax, dx				}	/*eax = rgbrgb*/	\
				{	and		eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
				{	mov		bx, dx				}	/*ebx = ...rgb*/	\
				{	shl		ebx, 16				}	/*ebx = rgb...*/	\
				{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
				{	movd	edx, mm3			}	/*	nAlpha    */	\
				{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
                {   sub     eax, ebx            }   /*eax = c1 - c2*/   \
				{	imul    eax, edx			}	/*eax = (c1 - c2)*nAlpha */	\
				{	shr		eax, 5				}	/*c=(c1 - c2)*nAlpha/32*/ \
                {   add     eax, ebx            }   /*c=(c1 - c2)*nAlpha/32 + c2*/ \
				{	and     eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov     dx, ax				}	/*edx = ...r.b*/	\
				{	shr     eax, 16				}	/*eax = ....g.*/	\
				{	add		edi, 2				}						\
				{	or      ax, dx				}	/*eax = ...rgb*/	\
				{	mov		[edi - 2], ax		}

#define		mix_2_pixel_color_alpha_font		\
				{	movd	mm5, ecx    		}						\
				{	movd    edx, mm4			}	/*edx = ...rgb*/	\
				{	movd	ecx, mm2    		}   /* nMask32 */		\
				{	mov		ax, dx				}	/*eax = ...rgb*/	\
				{	shl		eax, 16				}	/*eax = rgb...*/	\
				{	mov		ax, dx				}	/*eax = rgbrgb*/	\
				{	and		eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov		dx, [edi]			}	/*edx = ...rgb*/	\
				{	mov		bx, dx				}	/*ebx = ...rgb*/	\
				{	shl		ebx, 16				}	/*ebx = rgb...*/	\
				{	mov		bx, dx				}	/*ebx = rgbrgb*/	\
				{	and		ebx, ecx			}	/*ebx = .g.r.b*/	\
				{	movd	edx, mm3			}	/*	nAlpha    */	\
                {   sub     eax, ebx            }   /*eax = c1 - c2*/   \
				{	imul    eax, edx			}	/*eax = (c1 - c2)*nAlpha */	\
				{	shr		eax, 5				}	/*c=(c1 - c2)*nAlpha/32*/ \
                {   add     eax, ebx            }   /*c=(c1 - c2)*nAlpha/32 + c2*/ \
				{	and     eax, ecx			}	/*eax = .g.r.b*/	\
				{	mov     dx, ax				}	/*edx = ...r.b*/	\
				{	shr     eax, 16				}	/*eax = ....g.*/	\
				{	add		edi, 2				}						\
				{	or      ax, dx				}	/*eax = ...rgb*/	\
				{	movd    ecx, mm5    		}                       \
				{	mov		[edi - 2], ax		}

#define		mix_2_pixel_color_alpha_font_32						\
				{	movd	mm6, ecx    		}						\
				{	movd	edx, mm2			}	/*	nAlpha    */	\
				{	movd	eax, mm3			}	\
				{	imul	eax, edx            }   /*nAlpha*color*/\
				{	mov		ebx, [edi]			}	/*bgcolor*/\
				{	shr		ebx, 16				}	\
				{	and		ebx, 0xff			}	\
				{	mov		ecx, 255			}	\
				{	sub		ecx, edx			}	/*(255-nAlpha)*/\
				{	imul	ecx, ebx            }   /*(255-nAlpha)*color*/\
				{	add		eax, ecx			}	/*nAlpha*c1+(255-nAlpha)*c2*/\
				{	shr		eax, 8				}   /*(nAlpha*c1+(255-nAlpha)*c2)/255*/\
				{	shl		eax, 16				}	\
				{	push	eax                 }   \
				{	movd	eax, mm4			}	\
				{	imul	eax, edx            }   /*nAlpha*color*/\
				{	mov		ebx, [edi]			}	/*bgcolor*/\
				{	shr		ebx, 8				}	\
				{	and		ebx, 0xff			}	\
				{	mov		ecx, 255			}	\
				{	sub		ecx, edx			}	/*(255-nAlpha)*/\
				{	imul	ecx, ebx            }   /*(255-nAlpha)*color*/\
				{	add		eax, ecx			}	/*nAlpha*c1+(255-nAlpha)*c2*/\
				{	shr		eax, 8				}   /*(nAlpha*c1+(255-nAlpha)*c2)/255*/\
				{	shl		eax, 8				}	\
				{	push	eax                 }   \
				{	movd	eax, mm5			}	\
				{	imul	eax, edx            }   /*nAlpha*color*/\
				{	mov		ebx, [edi]			}	/*bgcolor*/\
				{	and		ebx, 0xff			}	\
				{	mov		ecx, 255			}	\
				{	sub		ecx, edx			}	/*(255-nAlpha)*/\
				{	imul	ecx, ebx            }   /*(255-nAlpha)*color*/\
				{	add		eax, ecx			}	/*nAlpha*c1+(255-nAlpha)*c2*/\
				{	shr		eax, 8				}   /*(nAlpha*c1+(255-nAlpha)*c2)/255*/\
				{	pop		ebx                 }   \
				{	or		eax,ebx             }   \
				{	pop		ebx                 }   \
				{	or		eax,ebx             }   \
				{	or		eax,0xff000000      }   \
				{	add		edi, 4				}	\
				{	mov		[edi-4], eax		}	\
				{	movd    ecx, mm6    		}	
