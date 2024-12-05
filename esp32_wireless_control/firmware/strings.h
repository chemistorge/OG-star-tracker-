// strings.h
#ifndef STRINGS_H
#define STRINGS_H

#include <Arduino.h>

// 支持的语言枚举
enum Language {
    EN,
    CN,
    // 可以轻松添加更多语言
    LANG_COUNT
};

// 字符串ID枚举
enum StringID {
    // 状态消息
    STR_TRACKING_ON,
    STR_TRACKING_OFF,
    STR_IDLE,
    STR_SLEWING,
    STR_INVALID_EXPOSURE,
    STR_INVALID_DITHER,
    STR_CAPTURE_ON,
    STR_CAPTURE_OFF,
    STR_CAPTURE_ALREADY_ON,
    STR_CAPTURE_ALREADY_OFF,
    STR_CAPTURES_REMAINING,
    
    // 参数键名
    STR_TRACKING_SPEED,
    STR_SPEED,
    STR_DIRECTION,
    STR_EXPOSURE,
    STR_NUM_EXPOSURES,
    STR_DITHER_ENABLED,
    STR_FOCAL_LENGTH,
    STR_PIXEL_SIZE,
    STR_DISABLE_TRACKING,
    
    // UI 元素
    STR_TITLE,
    STR_TRACKING,
    STR_HEMISPHERE,
    STR_NORTH,
    STR_SOUTH,
    STR_TRACKING_RATE,
    STR_SIDEREAL,
    STR_SOLAR,
    STR_LUNAR,
    STR_SLEW_CONTROL,
    STR_SPEED_MULTIPLIER,
    STR_CUSTOM,
    STR_CUSTOM_SPEED,
    STR_SLEW_HINT,
    STR_SLEW_LEFT,
    STR_SLEW_RIGHT,
    STR_ABORT_SLEW,
    STR_PHOTO_CONTROL,
    STR_EXPOSURE_LENGTH,
    STR_EXPOSURE_HINT,
    STR_NUM_EXPOSURES_HINT,
    STR_DITHER_SETTINGS,
    STR_DITHER_ENABLE,
    STR_FOCAL_LENGTH_HINT,
    STR_PIXEL_SIZE_HINT,
    STR_START_CAPTURE,
    STR_ABORT_CAPTURE,
    STR_STATUS,
    STR_STATUS_MSG,
    STR_FIRMWARE_VERSION,
    STR_BTN_ON,
    STR_BTN_OFF,
    STR_LANG_CN,
    
    STR_COUNT
};

// 获取字符串的函数
const char* getString(StringID id, Language lang = EN);

// 获取参数键名的函数
const char* getParamKey(StringID id);

// MIME类型常量
extern const char* MIME_TYPE_TEXT;
extern const char* MIME_TYPE_HTML;

// HTML模板
extern const char* html_template;
extern const char* LOGO_BASE64;

// 获取完整HTML的函数
String getHTML();

#define PLACEHOLDER_TITLE "TITLE"
#define PLACEHOLDER_TRACKING "TRACKING"
// ...其他占位符定义

class HTMLBuilder {
public:
    static String build(const char* tmpl, Language lang);
private:
    static void replaceAll(String& html, Language lang);
};

#endif
