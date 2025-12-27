//
// Created by JasonGu on 2025/12/27.
//

#ifndef MY_APPLICATION_TEXTRENDERER_H
#define MY_APPLICATION_TEXTRENDERER_H

#include <vector>
#include <map>
#include <fstream>
#include "stb_truetype.h"
#include "stb_rect_pack.h"

// 存储单个字符的元数据，用于后续生成顶点
struct CharGlyph {
    float u0, v0;     // 纹理坐标左上角
    float u1, v1;     // 纹理坐标右下角

    float xAdvance;   // 光标前进距离
    float xBearing;   // 左边距 (字形原点相对于 Grid 的偏移)
    float yBearing;   // 顶边距

    float width;      // 字形宽度
    float height;     // 字形高度
};

// 字体图集类
class FontAtlas {
public:
    int atlasWidth = 512;   // 图集宽度（像素）
    int atlasHeight = 512;  // 图集高度（像素）
    unsigned char* pixels = nullptr; // 纹理数据 (8位灰度)
    std::map<int, CharGlyph> glyphs; // key: unicode char, value: glyph info

    ~FontAtlas() {
        delete[] pixels;
    }
};

class TextRenderer {
    static bool LoadFontFromFile(FontAtlas& atlas, const char* filename, float fontSize) {
        // 1. 读取整个 TTF 文件
        std::ifstream file(filename, std::ios::binary | std::ios::ate);
        size_t fileSize = file.tellg();
        std::vector<char> ttfBuffer(fileSize);
        if (!file.is_open()) return false;
        file.seekg(0, std::ios::beg);
        file.read(ttfBuffer.data(), fileSize);
        file.close();

        // 2. 初始化 stbtt
        stbtt_fontinfo font;
        if (!stbtt_InitFont(&font, reinterpret_cast<const unsigned char*>(ttfBuffer.data()), 0)) {
            return false;
        }

        // 3. 计算缩放比例
        // stbtt_ScaleForPixelHeight 根据需要的字体高度计算缩放因子
        float scale = stbtt_ScaleForPixelHeight(&font, fontSize);

        // 4. 初始化图集打包器
        // 为图集分配内存 (初始全黑)
        atlas.pixels = new unsigned char[atlas.atlasWidth * atlas.atlasHeight];
        memset(atlas.pixels, 0, atlas.atlasWidth * atlas.atlasHeight);

        stbrp_context packContext;
        // 节点数组数量通常等于图集宽度，用于打包算法计算
        std::vector<stbrp_node> packNodes(atlas.atlasWidth);
        stbrp_init_target(&packContext, atlas.atlasWidth, atlas.atlasHeight, packNodes.data(), packNodes.size());

        // 5. 确定要加载的字符范围 (例如 ASCII 32-126)
        int firstChar = 32;
        int charCount = 95;
        std::vector<stbrp_rect> packRects(charCount);

        // 准备矩形数据
        for (int i = 0; i < charCount; i++) {
            int glyphIndex = stbtt_FindGlyphIndex(&font, firstChar + i);
            int ix0, iy0, ix1, iy1;
            // 获取字形包围盒
            stbtt_GetGlyphBitmapBox(&font, glyphIndex, scale, scale, &ix0, &iy0, &ix1, &iy1);

            int w = ix1 - ix0;
            int h = iy1 - iy0;

            packRects[i].id = firstChar + i; // 存入字符ID
            packRects[i].w = w;
            packRects[i].h = h;
            // x, y 初始化为0，稍后由 packer 计算
        }

        // 6. 执行打包计算
        stbrp_pack_rects(&packContext, packRects.data(), charCount);

        // 7. 光栅化并写入图集
        for (int i = 0; i < charCount; i++) {
            stbrp_rect& rect = packRects[i];
            if (rect.w == 0 || rect.h == 0 || !rect.was_packed) continue;

            int charCode = rect.id;
            int glyphIndex = stbtt_FindGlyphIndex(&font, charCode);

            CharGlyph glyph;

            // --- A. 生成位图 ---
            // 创建一个临时 buffer 存放当前字符的位图
            // 注意：这里也可以直接用 STBTT_malloc 等自定义分配器，或者利用 stride 直接写进大图集
            int w = rect.w;
            int h = rect.h;
            std::vector<unsigned char> tempBitmap(w * h);

            int ix0, iy0, ix1, iy1;
            stbtt_GetGlyphBitmapBox(&font, glyphIndex, scale, scale, &ix0, &iy0, &ix1, &iy1);

            // 渲染字形到 tempBitmap
            stbtt_MakeGlyphBitmap(&font, tempBitmap.data(), w, h, w, scale, scale, glyphIndex);

            // 将 tempBitmap 拷贝到大图集 atlas.pixels 的对应位置
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int atlasX = rect.x + x;
                    int atlasY = rect.y + y;
                    if (atlasX < atlas.atlasWidth && atlasY < atlas.atlasHeight) {
                        atlas.pixels[atlasY * atlas.atlasWidth + atlasX] = tempBitmap[y * w + x];
                    }
                }
            }

            // --- B. 获取度量信息 ---
            int advance, lsb; // left-side-bearing
            stbtt_GetGlyphHMetrics(&font, glyphIndex, &advance, &lsb);

            // --- C. 填充 Glyph 结构体 ---
            // UV 坐标 (归一化 0.0 - 1.0)
            glyph.u0 = (float)rect.x / atlas.atlasWidth;
            glyph.v0 = (float)rect.y / atlas.atlasHeight;
            glyph.u1 = (float)(rect.x + w) / atlas.atlasWidth;
            glyph.v1 = (float)(rect.y + h) / atlas.atlasHeight;

            // 像素尺寸
            glyph.width = (float)w;
            glyph.height = (float)h;

            // 偏移量 (用于定位 Quad)
            glyph.xBearing = (float)ix0;
            glyph.yBearing = (float)iy0; // 注意：stb 中 y 是向上的吗？在屏幕坐标系中通常需要翻转，这里先按原始值

            // 前进距离 (Scale 后)
            glyph.xAdvance = advance * scale;

            atlas.glyphs[charCode] = glyph;
        }

        return true;
    }
};


#endif //MY_APPLICATION_TEXTRENDERER_H
