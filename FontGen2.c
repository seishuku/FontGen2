#include <windows.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "math.h"
#include "list.h"

#define GYLP ('G'|('Y'<<8)|('L'<<16)|('P'<<24))

float fixed_to_float(const FIXED p)
{
	return ((p.value<<16)+p.fract)/65536.0f;
}

List_t Path;

void GetGlyph(HDC hDC, UINT Code)
{
	GLYPHMETRICS gm;
	BYTE *buff=NULL;
	TTPOLYGONHEADER *phdr=NULL;
	TTPOLYCURVE *pcur=NULL;
	DWORD pBytesCount;
	int i, j, Size, byteCount=0;
	const MAT2 Matrix={ { 0, 1 }, { 0, 0 }, { 0, 0 }, { 0, 1 } };

	Size=GetGlyphOutline(hDC, Code, GGO_BEZIER, &gm, 0, NULL, &Matrix);

	if(Size==0||Size==GDI_ERROR)
		return;

	buff=malloc(Size);

	if(buff==NULL)
		return;

	GetGlyphOutline(hDC, Code, GGO_BEZIER, &gm, Size, buff, &Matrix);

	phdr=(TTPOLYGONHEADER *)buff;

	while(byteCount<Size)
	{
		vec2 last, p[4];

		Vec2_Set(last, fixed_to_float(phdr->pfxStart.x), fixed_to_float(phdr->pfxStart.y));

		pBytesCount=sizeof(TTPOLYGONHEADER);
		pcur=(TTPOLYCURVE *)(phdr+1);

		while(pBytesCount<phdr->cb)
		{
			switch(pcur->wType)
			{
			case TT_PRIM_QSPLINE:
				break;

			case TT_PRIM_CSPLINE:
				for(j=0;j<pcur->cpfx;j+=3)
				{
					Vec2_Setv(p[0], last);

					for(i=0;i<3;++i)
						Vec2_Set(p[i+1], fixed_to_float(pcur->apfx[j+i].x), fixed_to_float(pcur->apfx[j+i].y));

					List_Add(&Path, &p[0]);
					List_Add(&Path, &p[1]);
					List_Add(&Path, &p[2]);
					List_Add(&Path, &p[3]);

					Vec2_Setv(last, p[3]);
				}
				break;

			case TT_PRIM_LINE:
				for(j=0;j<pcur->cpfx;j++)
				{
					Vec2_Set(p[0], fixed_to_float(pcur->apfx[j].x), fixed_to_float(pcur->apfx[j].y));

					List_Add(&Path, &last);
					List_Add(&Path, &last);
					List_Add(&Path, &p[0]);
					List_Add(&Path, &p[0]);

					Vec2_Setv(last, p[0]);
				}
				break;
			}

			pBytesCount+=sizeof(POINTFX)*pcur->cpfx+4;
			pcur=(TTPOLYCURVE *)((char *)pcur+sizeof(POINTFX)*pcur->cpfx+4);
		}

		byteCount+=phdr->cb;
		phdr=(TTPOLYGONHEADER *)((char *)phdr+phdr->cb);
	}

	free(buff);
}

int CheckParm(char *parm, int count, int argc, char **argv)
{
	int i;

	for(i=1;i<argc;i++)
	{
		if(_strnicmp(parm, argv[i], count)==0)
			return i;
	}

	return 0;
}

int main(int argc, char **argv)
{
	int i, bold=0, italic=0, size=16;
	char *FontName=NULL;
	char *OutputName=NULL;

	if((i=CheckParm("-f", 2, argc, argv))!=0)
		FontName=argv[i+1];

	if((i=CheckParm("-o", 2, argc, argv))!=0)
		OutputName=argv[i+1];

	if((i=CheckParm("-b", 2, argc, argv))!=0)
		bold=1;

	if((i=CheckParm("-i", 2, argc, argv))!=0)
		italic=1;

	if((i=CheckParm("-s", 2, argc, argv))!=0)
		size=atoi(argv[i+1]);

	if((i=CheckParm("-?", 2, argc, argv))!=0)
	{
		fprintf(stderr, "Help:\n"
						"-f \"Font Name\" : Sets font name (defaults to \"NULL\")\n"
						"-o \"output.gylph\" : Sets the output file name (defaults to \"font.gylph\")\n"
						"-b : Sets Bold\n"
						"-i : Sets Italic\n"
						"-s : Sets height (defaults to 16)\n\n"
						"Standard Usage:\n"
						"%s -f \"Courier New\"\n\n", argv[0]);
		return -1;
	}

	FILE *Stream=fopen((OutputName==NULL)?"font.gylph":OutputName, "wb");

	if(!Stream)
	{
		fprintf(stderr, "Open file failed.\n");
		return -2;
	}

	HDC hDC=CreateCompatibleDC(NULL);

	if(!hDC)
	{
		fclose(Stream);
		fprintf(stderr, "CreateCompatibleDC failed.\n");
		return -2;
	}

	HFONT hFont=CreateFont(size, 0, 0, 0, bold?FW_BOLD:FW_NORMAL, italic, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH, FontName);

	if(!hFont)
	{
		fclose(Stream);
		DeleteDC(hDC);
		fprintf(stderr, "CreateFont failed.\n");
		return -2;
	}

	SelectObject(hDC, hFont);

	List_Init(&Path, sizeof(vec2), 0, NULL);

	uint32_t magic=GYLP;

	fwrite(&magic, sizeof(uint32_t), 1, Stream);
	fwrite(&size, sizeof(uint32_t), 1, Stream);

	for(i=0;i<=255;i++)
	{
		GetGlyph(hDC, i);

		uint32_t Count=(uint32_t)List_GetCount(&Path);
		uint32_t Advance=0;

		GetCharWidth32(hDC, i, i, &Advance);

		fwrite(&Advance, sizeof(uint32_t), 1, Stream);
		fwrite(&Count, sizeof(uint32_t), 1, Stream);
		fwrite(List_GetPointer(&Path, 0), sizeof(vec2), Count, Stream);

		List_Clear(&Path);
	}

	fclose(Stream);

	List_Destroy(&Path);

	DeleteObject(hFont);
	DeleteDC(hDC);

	return 0;
}