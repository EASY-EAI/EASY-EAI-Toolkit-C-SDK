#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "bmp_opt.h"


uint32_t read_BMP_file(unsigned char *pOutputPIC, char *filename)
{
	
    if(!pOutputPIC){
		printf("pointor is null...\n");
		return -1;
    }
		
    int bmp = open(filename, O_RDONLY, 0644);
    if(bmp < 0){
		printf("%s open faild ...\n", filename);
		return -1;
    }
	
	struct bmp_header header;
	printf("head size: %d\n", sizeof(struct bmp_header));
	memset(&header, 0, sizeof(struct bmp_header));
    int ret;
    //读出bmp文件头的数据
    ret = read(bmp , &header , sizeof(struct bmp_header));
#if 0	
	//以下是bmp图的相关数据
    printf(" Signatue[0]      : %c  \n " , header.Signatue[0]  );
    printf(" Signatue[1]      : %c  \n " , header.Signatue[1]  );
    printf(" FileSize         : 0x%x  \n " , header.FileSize     );
    printf(" Reserv1          : 0x%x  \n " , header.Reserv1      );
    printf(" Reserv2          : 0x%x  \n " , header.Reserv2      );
    printf(" FileOffset       : 0x%x  \n " , header.FileOffset   );
    printf(" DIBHeaderSize    : 0x%x  \n " , header.DIBHeaderSize);
    printf(" ImageWidth       : 0x%x  \n " , header.ImageWidth   );
    printf(" ImageHight       : 0x%x  \n " , header.ImageHight   );
    printf(" Planes           : 0x%x  \n " , header.Planes       );
    printf(" BPP              : 0x%x  \n " , header.BPP          );
    printf(" Compression      : 0x%x  \n " , header.Compression  );
    printf(" ImageSize        : 0x%x  \n " , header.ImageSize    );
    printf(" XPPM             : 0x%x  \n " , header.XPPM         );
    printf(" YPPM             : 0x%x  \n " , header.YPPM         );
    printf(" CCT              : 0x%x  \n " , header.CCT          );
    printf(" ICC              : 0x%x  \n " , header.ICC          );
#endif
	//读出图片
	read(bmp , pOutputPIC, header.ImageWidth * header.ImageHight *(header.BPP>>3));

    close(bmp);
	return 0;
}

static unsigned char sg_BHeader[] = {
    0x42, 0x4D, 0x36, 0x00, //0x00-0x04
    0x18, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x36, 0x00,
    0x00, 0x00, 0x28, 0x00, 
    0x00, 0x00, 0x00, 0x04, //0x10-0x14
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x01, 0x00,
    0x10, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00, //0x20-0x24
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00
};
uint32_t save_BMP_file(char *filename, uint8_t *pBuff, uint16_t width, uint16_t height, int Bpp)
{
    unsigned char *pOpt = pBuff;
	
    typedef unsigned int UINT;
    typedef unsigned char UCHAR;
    UINT m_Width = (UINT)width, m_Height = (UINT)height;
    int bmp = open(filename, O_WRONLY | O_CREAT, 0644);
    if(bmp < 0)
        return -1;
	
    sg_BHeader[0x02] = (UCHAR)(m_Width * m_Height * (Bpp>>3) + 0x36) & 0xff;
    sg_BHeader[0x03] = (UCHAR)((m_Width * m_Height * (Bpp>>3) + 0x36) >> 8) & 0xff;
    sg_BHeader[0x04] = (UCHAR)((m_Width * m_Height * (Bpp>>3) + 0x36) >> 16) & 0xff;
    sg_BHeader[0x05] = (UCHAR)((m_Width * m_Height * (Bpp>>3) + 0x36) >> 24) & 0xff;
    sg_BHeader[0x12] = (UCHAR)m_Width & 0xff;
    sg_BHeader[0x13] = (UCHAR)(m_Width >> 8) & 0xff;
    sg_BHeader[0x14] = (UCHAR)(m_Width >> 16) & 0xff;
    sg_BHeader[0x15] = (UCHAR)(m_Width >> 24) & 0xff;
    sg_BHeader[0x16] = (UCHAR)m_Height & 0xff;
    sg_BHeader[0x17] = (UCHAR)(m_Height >> 8) & 0xff;
    sg_BHeader[0x18] = (UCHAR)(m_Height >> 16) & 0xff;
    sg_BHeader[0x19] = (UCHAR)(m_Height >> 24) & 0xff;	
    sg_BHeader[0x1C] = (UCHAR)Bpp& 0xff;
    sg_BHeader[0x1D] = (UCHAR)(Bpp >> 8) & 0xff;
    write(bmp, sg_BHeader, sizeof(sg_BHeader));
#if 0
	unsigned int *pColour = (unsigned int *)p;

	//ABGR transform too ARGB
    unsigned char alpha,red,green,blue;
	unsigned int colour = 0x000000FF;
    alpha = (colour & 0xff000000) >> 24;
    blue = (colour & 0x00ff0000) >> 16;
    green = (colour & 0x0000ff00) >> 8;
    red = (colour & 0x000000ff);
	
    char *pTcolour = (char *)&colour;
    memcpy(pTcolour++, &blue,1);
    memcpy(pTcolour++, &green,1);
    memcpy(pTcolour++, &red,1);
	memcpy(pTcolour++, &alpha,1);
	
	//按矩形宽高填色
	int i;
	for(i = 0; i < m_Height * m_Width; i++){
		memcpy(pColour++, &colour, sizeof(colour));
	}
#endif

	write(bmp, pOpt, m_Width * m_Height * (Bpp>>3));

    close(bmp);
	
	return 0;
}
