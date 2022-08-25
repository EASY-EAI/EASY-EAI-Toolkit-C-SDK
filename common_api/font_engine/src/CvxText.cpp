/**
 *
 * Copyright 2021 by Guangzhou Easy EAI Technologny Co.,Ltd.
 * website: www.easy-eai.com
 *
 * Author: Jiehao.Zhong <zhongjiehao@easy-eai.com>
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * License file for more details.
 * 
 */
 
#include <sys/time.h>

#include <wchar.h>
#include <assert.h>
#include <locale.h>
#include <ctype.h>
#include <cmath>

#include "CvxText.h"

#include "font_engine.h"
 
// 打开字库
CvxText::CvxText(const char* freeType)
{
    assert(freeType != NULL);
 
    // 打开字库文件, 创建一个字体
    if(FT_Init_FreeType(&m_library)) throw;
    if(FT_New_Face(m_library, freeType, 0, &m_face)) throw;
 
    // 设置字体输出参数
    restoreFont();
 
    // 设置C语言的字符集环境
    setlocale(LC_ALL, "");
}
 
// 释放FreeType资源
CvxText::~CvxText()
{
    FT_Done_Face(m_face);
    FT_Done_FreeType(m_library);
}
 
// 设置字体参数:
//
// font         - 字体类型, 目前不支持
// size         - 字体大小/空白比例/间隔比例/旋转角度
// underline   - 下画线
// diaphaneity   - 透明度
void CvxText::getFont(int* type, cv::Scalar* size, bool* underline, float* diaphaneity)
{
    if (type) *type = m_fontType;
    if (size) *size = m_fontSize;
    if (underline) *underline = m_fontUnderline;
    if (diaphaneity) *diaphaneity = m_fontDiaphaneity;
}
 
void CvxText::setFont(int* type, cv::Scalar* size, bool* underline, float* diaphaneity)
{
    // 参数合法性检查
    if (type) {
        if(type >= 0) m_fontType = *type;
    }
    if (size) {
        m_fontSize.val[0] = std::fabs(size->val[0]);
        m_fontSize.val[1] = std::fabs(size->val[1]);
        m_fontSize.val[2] = std::fabs(size->val[2]);
        m_fontSize.val[3] = std::fabs(size->val[3]);
    }
    if (underline) {
        m_fontUnderline   = *underline;
    }
    if (diaphaneity) {
        m_fontDiaphaneity = *diaphaneity;
    }
 
    FT_Set_Pixel_Sizes(m_face, (int)m_fontSize.val[0], 0);
}
 
// 恢复原始的字体设置
void CvxText::restoreFont()
{
    m_fontType = 0;            // 字体类型(不支持)
 
    m_fontSize.val[0] = 20;      // 字体大小
    m_fontSize.val[1] = 0.5;   // 空白字符大小比例
    m_fontSize.val[2] = 0.1;   // 间隔大小比例
    m_fontSize.val[3] = 0;      // 旋转角度(不支持)
 
    m_fontUnderline   = false;   // 下画线(不支持)
 
    m_fontDiaphaneity = 1.0;   // 色彩比例(可产生透明效果)
 
    // 设置字符大小
    FT_Set_Pixel_Sizes(m_face, (int)m_fontSize.val[0], 0);
}
 
// 输出函数(颜色默认为白色)
int CvxText::putText(cv::Mat& img, char* text, cv::Point pos)
{
    return putText(img, text, pos, CV_RGB(255, 255, 255));
}
 
int CvxText::putText(cv::Mat& img, const wchar_t* text, cv::Point pos)
{
    return putText(img, text, pos, CV_RGB(255,255,255));
}
 
int CvxText::putText(cv::Mat& img, const char* text, cv::Point pos, cv::Scalar color)
{
    if (img.data == nullptr) return -1;
    if (text == nullptr) return -1;
 
    int i;
    for (i = 0; text[i] != '\0'; ++i) {
        wchar_t wc = text[i];
 
        // 解析双字节符号
        if(!isascii(wc)) mbtowc(&wc, &text[i++], 2);
 
        // 输出当前的字符
        putWChar(img, wc, pos, color);
    }
 
    return i;
}
 
int CvxText::putText(cv::Mat& img, const wchar_t* text, cv::Point pos, cv::Scalar color)
{
    if (img.data == nullptr) return -1;
    if (text == nullptr) return -1;
 
    int i;
    for(i = 0; text[i] != '\0'; ++i) {
        // 输出当前的字符
        putWChar(img, text[i], pos, color);
    }
 
    return i;
}
 
// 输出当前字符, 更新m_pos位置
void CvxText::putWChar(cv::Mat& img, wchar_t wc, cv::Point& pos, cv::Scalar color)
{
    // 根据unicode生成字体的二值位图
    FT_UInt glyph_index = FT_Get_Char_Index(m_face, wc);
    FT_Load_Glyph(m_face, glyph_index, FT_LOAD_DEFAULT);
    FT_Render_Glyph(m_face->glyph, FT_RENDER_MODE_MONO);
 
    FT_GlyphSlot slot = m_face->glyph;
 
    // 行列数
    int rows = slot->bitmap.rows;
    int cols = slot->bitmap.width;
 
    for (int i = 0; i < rows; ++i) {
        for(int j = 0; j < cols; ++j) {
            int off  = i * slot->bitmap.pitch + j/8;
 
            if (slot->bitmap.buffer[off] & (0xC0 >> (j%8))) {
                int r = pos.y - (rows-1-i);
                int c = pos.x + j;
 
                if(r >= 0 && r < img.rows && c >= 0 && c < img.cols) {
                    cv::Vec3b pixel = img.at<cv::Vec3b>(cv::Point(c, r));
                    cv::Scalar scalar = cv::Scalar(pixel.val[0], pixel.val[1], pixel.val[2]);
 
                    // 进行色彩融合
                    float p = m_fontDiaphaneity;
                    for (int k = 0; k < 4; ++k) {
                        scalar.val[k] = scalar.val[k]*(1-p) + color.val[k]*p;
                    }
 
                    img.at<cv::Vec3b>(cv::Point(c, r))[0] = (unsigned char)(scalar.val[0]);
                    img.at<cv::Vec3b>(cv::Point(c, r))[1] = (unsigned char)(scalar.val[1]);
                    img.at<cv::Vec3b>(cv::Point(c, r))[2] = (unsigned char)(scalar.val[2]);
                }
            }
        }
    }
 
    // 修改下一个字的输出位置
    double space = m_fontSize.val[0]*m_fontSize.val[1];
    double sep   = m_fontSize.val[0]*m_fontSize.val[2];
 
    pos.x += (int)((cols? cols: space) + sep);
}

static int ToWchar(char* &src, wchar_t* &dest, const char *locale)
{
    if (src == NULL) {
        dest = NULL;
        return 0;
    }
 
    // 根据环境变量设置locale
    char *res = setlocale(LC_CTYPE, locale);
	if(!res){
		printf("[%s] is unsupported by this system \n", locale);
		return -1;
	}
 
    // 得到转化为需要的宽字符大小
    int w_size = mbstowcs(NULL, src, 0) + 1;
	
    // w_size = 0 说明mbstowcs返回值为-1。即在运行过程中遇到了非法字符(很有可能是locale没有设置正确)
    if (w_size == 0) {
        dest = NULL;
        return -2;
    }
 
    //wcout << "w_size" << w_size << endl;
    dest = new wchar_t[w_size];
    if (!dest) {
        return -3;
    }
 
    int ret = mbstowcs(dest, src, strlen(src)+1);
    if (ret <= 0) {
        return -4;
    }
    return 0;
}

static uint64_t get_timeval_us()
{
    struct timeval tv;	
	gettimeofday(&tv, NULL);
	
	return ((uint64_t)tv.tv_sec * 1000000 + tv.tv_usec);
}
static uint64_t get_timeval_ms()
{
    struct timeval tv;	
	gettimeofday(&tv, NULL);
	
	return ((uint64_t)tv.tv_sec * 1000 + tv.tv_usec/1000);
}

char g_fontLib[128] = {0};   // 字体 [*.ttf]
char g_charSet[64] = {0};   // 字符集 [zh_CN.utf8]
uint32_t g_fontSize = 0;    // 字体大小

int32_t font_engine_init(char *fontLibPath, char *charSet)
{
	int charLen = 0;
	
	charLen = strlen(fontLibPath) >= (sizeof(g_fontLib)-1) ? (sizeof(g_fontLib)-1) : strlen(fontLibPath);
	memcpy(g_fontLib, fontLibPath, charLen);
	
	charLen = strlen(charSet) >= (sizeof(g_charSet)-1) ? (sizeof(g_charSet)-1) : strlen(charSet);
	memcpy(g_charSet, charSet, charLen);
	
	return 0;
}

int32_t font_engine_set_fontSize(uint32_t fontSize)
{
	g_fontSize = fontSize;
	return 0;
}

int32_t font_engine_putText(uint8_t *imgData, uint32_t imgWidth, uint32_t imgHeight, char *text, uint32_t posX, uint32_t posY, FontColor color)
{	
	cv::Mat image;

	if((NULL == imgData) || (NULL == text)){
		printf("[error]: input data error, imgData or text is empty \n");
		return -1;
	}
	
	if((0 == strlen(g_fontLib)) || (0 == strlen(g_charSet))){
		printf("[error]: font engine is not init \n");
		return -1;
	}
	
	image = cv::Mat(imgHeight, imgWidth, CV_8UC3, imgData);
	
	CvxText cvxText(g_fontLib);	//根据字库创建字体
	
    cv::Scalar size1{ (double)g_fontSize, 0.5, 0.1, 0 }; // (字体大小, 无效的, 字符间距, 无效的 }
	float alpha = (float)color.Alpha/255;
    cvxText.setFont(nullptr, &size1, nullptr, &alpha);

    wchar_t *w_str;
    int ret = ToWchar(text, w_str, g_charSet);
	if(w_str){
		cvxText.putText(image, w_str, cv::Point(posX, (posY+g_fontSize)), cv::Scalar(color.Bule, color.Green, color.Red));
		delete [] w_str; 
	}
	
	return 0;
}


int32_t font_engine_unInit()
{	
	memset(g_charSet, 0, sizeof(g_charSet));
	
	memset(g_fontLib, 0, sizeof(g_fontLib));
	
	return 0;
}





