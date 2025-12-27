//
// Created by JasonGu on 2025/12/27.
//
// src/stb_impl.cpp
// 避免符号重定义错误
// 在这里定义实现宏，只在这个文件里生效
#define STB_TRUETYPE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_RECT_PACK_IMPLEMENTATION
// ... 需要用哪个就 define 哪个