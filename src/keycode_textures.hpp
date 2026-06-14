#ifndef KEYCODE_TEXTURES_HPP
#define KEYCODE_TEXTURES_HPP

#include <godot_cpp/classes/texture2d.hpp>
#include <godot_cpp/classes/global_constants.hpp>
#include <unordered_map>

#include "preload.hpp"

namespace godot {

    struct KeyTextureData {
        Key key;
        Preloaded<Texture2D> texture;

        KeyTextureData() = default;
        KeyTextureData(Key p_key, const String& path)
            : key(p_key), texture(preload<Texture2D>(path)) {
        }
    };

    class KeycodeTextureList {
    public:
        static KeycodeTextureList& get_singleton() {
            static KeycodeTextureList instance;
            return instance;
        }

        // 获取按键对应的纹理
        Ref<Texture2D> get_texture(Key key) {
            auto it = texture_map_.find(key);
            if (it != texture_map_.end()) {
                return it->second.get();
            }
            return Ref<Texture2D>();
        }

        // 获取所有数据
        const std::vector<KeyTextureData>& get_all() const {
            return texture_list_;
        }

        // 检查是否包含某个按键
        bool has_key(Key key) const {
            return texture_map_.find(key) != texture_map_.end();
        }

    private:
        KeycodeTextureList() {
            initialize();
        }

        void initialize() {
            // 使用 emplace_back 避免拷贝
            texture_list_.reserve(128);

            // 特殊键
            add(Key::KEY_NONE, "uid://dbp57bl4bms3i");
            add(Key::KEY_SPECIAL, "uid://0md0e4fro3ym");
            add(Key::KEY_ESCAPE, "uid://bfw4eym2krb58");
            add(Key::KEY_TAB, "uid://blaexxatmfdsp");
            add(Key::KEY_BACKTAB, "uid://8f47smwe0111");
            add(Key::KEY_BACKSPACE, "uid://n2b4jinprm32");
            add(Key::KEY_ENTER, "uid://doelafodrfvd2");
            add(Key::KEY_KP_ENTER, "uid://cj21dposdi2r7");
            add(Key::KEY_INSERT, "uid://vbhvtxstn8g8");
            add(Key::KEY_DELETE, "uid://cwb7f0ei5i7up");
            add(Key::KEY_PAUSE, "uid://dwb4v2jqo0s8q");
            add(Key::KEY_PRINT, "uid://xxbrdbdiwvve");
            add(Key::KEY_SYSREQ, "uid://b4v4g87200i12");
            add(Key::KEY_CLEAR, "uid://tj60ju86yofi");
            add(Key::KEY_HOME, "uid://dbp57bl4bms3i");
            add(Key::KEY_END, "uid://dk10nxodlfweu");
            add(Key::KEY_LEFT, "uid://bh30ife406kre");
            add(Key::KEY_UP, "uid://bkcfadf64mc3o");
            add(Key::KEY_RIGHT, "uid://b415jpmlix8bm");
            add(Key::KEY_DOWN, "uid://gfmc0oli2bff");
            add(Key::KEY_PAGEUP, "uid://bc0oxh2j8c5fi");
            add(Key::KEY_PAGEDOWN, "uid://byxeii4pbbswc");
            add(Key::KEY_SHIFT, "uid://bicf6gj220ics");
            add(Key::KEY_CTRL, "uid://cclxqm7j7c5y");
            add(Key::KEY_META, "uid://cncmxuqvh306y");
            add(Key::KEY_ALT, "uid://dg2fqbmuqakwr");
            add(Key::KEY_CAPSLOCK, "uid://dv3cgandur5dp");
            add(Key::KEY_NUMLOCK, "uid://ca1qe1lvrvvbl");
            add(Key::KEY_SCROLLLOCK, "uid://5b0tm4k2j0ur");

            // F1-F35
            add(Key::KEY_F1, "uid://cc26upja3q5px");
            add(Key::KEY_F2, "uid://cd2cnr7l07dx6");
            add(Key::KEY_F3, "uid://ce1hhgw4ru72m");
            add(Key::KEY_F4, "uid://cf0maj3ovg1jm");
            add(Key::KEY_F5, "uid://c57tetr2t6ve4");
            add(Key::KEY_F6, "uid://dbs0wcfi882ng");
            add(Key::KEY_F7, "uid://c66x6wu6nm3ep");
            add(Key::KEY_F8, "uid://d2fveap8mnlao");
            add(Key::KEY_F9, "uid://d2wgsgg5d81g6");
            add(Key::KEY_F10, "uid://g74j02rcd05x");
            add(Key::KEY_F11, "uid://bvgypan1cm2bk");
            add(Key::KEY_F12, "uid://didrjk6euax1q");
            add(Key::KEY_F13, "uid://nqjv70vpw1u8");
            add(Key::KEY_F14, "uid://bduy4oye6248h");
            add(Key::KEY_F15, "uid://b20lkm75c8xu8");
            add(Key::KEY_F16, "uid://cv8q3giicd8oi");
            add(Key::KEY_F17, "uid://dmen8vmbi277t");
            add(Key::KEY_F18, "uid://g3uagrepsy5t");
            add(Key::KEY_F19, "uid://58wm7pn7250k");
            add(Key::KEY_F20, "uid://b36o41itcj72q");
            add(Key::KEY_F21, "uid://bqa32qxhp1l2a");
            add(Key::KEY_F22, "uid://bcu4x8ufxsj5u");
            add(Key::KEY_F23, "uid://luyv6pj3t8yn");
            add(Key::KEY_F24, "uid://e50a0bgnsqav");
            add(Key::KEY_F25, "uid://d24n5jod08ipx");
            add(Key::KEY_F26, "uid://dvfphx8l0syfw");
            add(Key::KEY_F27, "uid://dopqqwcg8m2ya");
            add(Key::KEY_F28, "uid://dh0rym0ls62lv");
            add(Key::KEY_F29, "uid://dbkj6qoxwrsli");
            add(Key::KEY_F30, "uid://chdso0fk07y1r");
            add(Key::KEY_F31, "uid://cwx2jt4xp5c7q");
            add(Key::KEY_F32, "uid://dekb634wiwsvc");
            add(Key::KEY_F33, "uid://dt5k0jhw2lah1");
            add(Key::KEY_F34, "uid://ca0k530oxpqa1");
            add(Key::KEY_F35, "uid://cispjm86s1qvm");

            // 小键盘
            add(Key::KEY_KP_MULTIPLY, "uid://dvt2xjg1a83jh");
            add(Key::KEY_KP_DIVIDE, "uid://dn5m4e23p84he");
            add(Key::KEY_KP_SUBTRACT, "uid://lino24xfq2p7");
            add(Key::KEY_KP_ADD, "uid://dxjxtwlblilk3");
            add(Key::KEY_KP_0, "uid://clkaidxwlq3ck");
            add(Key::KEY_KP_1, "uid://cmjfavrjqedfh");
            add(Key::KEY_KP_2, "uid://b522m378spyt7");
            add(Key::KEY_KP_3, "uid://c1q10d87n4wbp");
            add(Key::KEY_KP_4, "uid://0f2iabp0bw45");
            add(Key::KEY_KP_5, "uid://dbxf1bml46smv");
            add(Key::KEY_KP_6, "uid://dgjmjxrul57ov");
            add(Key::KEY_KP_7, "uid://dk4s2b1ii33ac");
            add(Key::KEY_KP_8, "uid://dhirdcy73gdlq");
            add(Key::KEY_KP_9, "uid://dh0cqu4dnkfsg");

            // 多媒体键
            add(Key::KEY_MENU, "uid://jxsiagf2wqcn");
            add(Key::KEY_HYPER, "uid://dafv26u2tmovo");
            add(Key::KEY_HELP, "uid://dpol1ioj7230x");
            add(Key::KEY_BACK, "uid://w2duylsvax0c");
            add(Key::KEY_FORWARD, "uid://bywg3u655lmhu");
            add(Key::KEY_STOP, "uid://dc3u5tlqmtymd");
            add(Key::KEY_REFRESH, "uid://balq742dk5w2r");
            add(Key::KEY_VOLUMEDOWN, "uid://dd0nss6qb74jj");
            add(Key::KEY_VOLUMEMUTE, "uid://2cfckk3ty0pt");
            add(Key::KEY_VOLUMEUP, "uid://emyqyhm7k117");
            add(Key::KEY_MEDIAPLAY, "uid://kh2aepbpilij");
            add(Key::KEY_MEDIASTOP, "uid://cjc0kxy6a5fq6");
            add(Key::KEY_MEDIAPREVIOUS, "uid://b3sgytkaarfa3");
            add(Key::KEY_MEDIANEXT, "uid://dgebexiouiubi");
            add(Key::KEY_MEDIARECORD, "uid://dw6ko7ddcl4bq");
            add(Key::KEY_HOMEPAGE, "uid://5cogeraigccy");
            add(Key::KEY_FAVORITES, "uid://c5d2xc7vfoqr");
            add(Key::KEY_SEARCH, "uid://bbpgk0lgblrom");
            add(Key::KEY_STANDBY, "uid://ds4fc8xu2g8uh");
            add(Key::KEY_OPENURL, "uid://dhind38d5kcvh");
            add(Key::KEY_LAUNCHMAIL, "uid://defwcy5v7b7xu");
            add(Key::KEY_LAUNCHMEDIA, "uid://b7kdpa6l75ya2");
            add(Key::KEY_LAUNCH0, "uid://cxdcb1m4mtexa");
            add(Key::KEY_LAUNCH1, "uid://b8oqgme7etfig");
            add(Key::KEY_LAUNCH2, "uid://ciu4gwim4cy5t");
            add(Key::KEY_LAUNCH3, "uid://cbm01p2if6vx1");
            add(Key::KEY_LAUNCH4, "uid://d03uf6dnjg6sc");
            add(Key::KEY_LAUNCH5, "uid://d12y8e7q6w64h");
            add(Key::KEY_LAUNCH6, "uid://cmrooo1mksxxi");
            add(Key::KEY_LAUNCH7, "uid://cnqthxjdfje0m");
            add(Key::KEY_LAUNCH8, "uid://de614isfinobl");
            add(Key::KEY_LAUNCH9, "uid://djr8m26nid6i1");
            add(Key::KEY_LAUNCHA, "uid://j8rkpkfm32tr");
            add(Key::KEY_LAUNCHB, "uid://pe7x5wt77a56");
            add(Key::KEY_LAUNCHC, "uid://co6hwe3syvdsj");
            add(Key::KEY_LAUNCHD, "uid://bvy0wbym1sqqg");
            add(Key::KEY_LAUNCHE, "uid://da76bhoexfixe");
            add(Key::KEY_LAUNCHF, "uid://c37mlhs1i4gug");
            add(Key::KEY_GLOBE, "uid://dqevx6nxwkwuo");
            add(Key::KEY_KEYBOARD, "uid://bwfkq26pd8bja");
            add(Key::KEY_JIS_EISU, "uid://c0p1pfpxv883b");
            add(Key::KEY_JIS_KANA, "uid://c1xt3q72ixll5");
            add(Key::KEY_UNKNOWN, "uid://bej0x2uiufrbc");

            // 符号键
            add(Key::KEY_SPACE, "uid://brukio0g7tn6q");
            add(Key::KEY_EXCLAM, "uid://tdulefrukhq5");
            add(Key::KEY_QUOTEDBL, "uid://bsy0nnagin1j6");
            add(Key::KEY_NUMBERSIGN, "uid://l0v8jqejaynb");
            add(Key::KEY_DOLLAR, "uid://c4sue38kd55wf");
            add(Key::KEY_PERCENT, "uid://djdybln224ygb");
            add(Key::KEY_AMPERSAND, "uid://disnr0k51uvy1");
            add(Key::KEY_APOSTROPHE, "uid://bt346gl8jjvk3");
            add(Key::KEY_PARENLEFT, "uid://drpxcp4o818qu");
            add(Key::KEY_PARENRIGHT, "uid://6quineak1b5g");
            add(Key::KEY_ASTERISK, "uid://ccrypuw0ffriq");
            add(Key::KEY_PLUS, "uid://c01avg16gvtb0");
            add(Key::KEY_COMMA, "uid://g1tby88ti5ja");
            add(Key::KEY_MINUS, "uid://balmyx05ia6hj");
            add(Key::KEY_PERIOD, "uid://4mt3x1stivj7");
            add(Key::KEY_SLASH, "uid://laf7euo0wfwb");

            // 数字键
            add(Key::KEY_0, "uid://siys0fg7f6vu");
            add(Key::KEY_1, "uid://bo8hm3klcnqeo");
            add(Key::KEY_2, "uid://b6607t2i53xxh");
            add(Key::KEY_3, "uid://cxvafi5kpckoj");
            add(Key::KEY_4, "uid://b6o8rqp3tt7r6");
            add(Key::KEY_5, "uid://f4prdd3lykxs");
            add(Key::KEY_6, "uid://c84fpjyt8oxh0");
            add(Key::KEY_7, "uid://ckh4hijqxn7aw");
            add(Key::KEY_8, "uid://bj1w3qqkxyryq");
            add(Key::KEY_9, "uid://brdxaeuuddcyh");

            // 更多符号
            add(Key::KEY_COLON, "uid://bxi8nkn4auli3");
            add(Key::KEY_SEMICOLON, "uid://bwx2ljqp3fq52");
            add(Key::KEY_LESS, "uid://nhwyw2jwdq6n");
            add(Key::KEY_EQUAL, "uid://b0t8j5ay2ofcv");
            add(Key::KEY_GREATER, "uid://b7i0vcr68i2s3");
            add(Key::KEY_QUESTION, "uid://c33gc7rud27k4");
            add(Key::KEY_AT, "uid://dg2fqbmuqakwr");

            // 字母键 A-Z
            add(Key::KEY_A, "uid://bln5j3i15tko8");
            add(Key::KEY_B, "uid://dg12c2wncshxu");
            add(Key::KEY_C, "uid://jsylhmi6vjom");
            add(Key::KEY_D, "uid://bh7e1lrbm0ckd");
            add(Key::KEY_E, "uid://cj0hj5ljhd1g4");
            add(Key::KEY_F, "uid://de7vjii5njm7n");
            add(Key::KEY_G, "uid://i8v0daa3x38p");
            add(Key::KEY_H, "uid://biqxngru8yiqa");
            add(Key::KEY_I, "uid://cg88wut80myle");
            add(Key::KEY_J, "uid://dgfbwdcyexfob");
            add(Key::KEY_K, "uid://hti4cyvseh3o");
            add(Key::KEY_L, "uid://xwfjktbl2fvp");
            add(Key::KEY_M, "uid://desveles0ficq");
            add(Key::KEY_N, "uid://duxfk565jue73");
            add(Key::KEY_O, "uid://ceua1far53240");
            add(Key::KEY_P, "uid://cvc25t746bvp6");
            add(Key::KEY_Q, "uid://bfet7i3sb0vfu");
            add(Key::KEY_R, "uid://bvtjmsbk7qp3p");
            add(Key::KEY_S, "uid://bajgyc2sgygi");
            add(Key::KEY_T, "uid://jdufc3lkmmoy");
            add(Key::KEY_U, "uid://bqameu17ndisr");
            add(Key::KEY_V, "uid://byc02cab4cjq8");
            add(Key::KEY_W, "uid://bao8sbxmjv065");
            add(Key::KEY_X, "uid://bi0tjf47hoia6");
            add(Key::KEY_Y, "uid://cqab4p4msyrxt");
            add(Key::KEY_Z, "uid://cymxj2ghct8ld");

            // 括号等
            add(Key::KEY_BRACKETLEFT, "uid://odknq8n5dege");
            add(Key::KEY_BACKSLASH, "uid://c62f3mbi0awla");
            add(Key::KEY_BRACKETRIGHT, "uid://bnmenp0y8gjfy");
            add(Key::KEY_ASCIICIRCUM, "uid://dl7e5qiyusm2e");
            add(Key::KEY_UNDERSCORE, "uid://r2ih82g7atnh");
            add(Key::KEY_QUOTELEFT, "uid://cpqiacobpxia1");
            add(Key::KEY_BRACELEFT, "uid://dc52og7aep2wl");
            add(Key::KEY_BAR, "uid://trori8j5sk4u");
            add(Key::KEY_BRACERIGHT, "uid://dorqllry5k7hi");
            add(Key::KEY_ASCIITILDE, "uid://px734vab5dgv");
            add(Key::KEY_YEN, "uid://bf36pmlhb0mij");
            add(Key::KEY_SECTION, "uid://dn76myhkcgtjd");
        }

        void add(Key key, const String& path) {
            texture_list_.emplace_back(key, path);
            texture_map_[key] = Preloaded<Texture2D>(path);
        }

        std::vector<KeyTextureData> texture_list_;
        std::unordered_map<Key, Preloaded<Texture2D>> texture_map_;
    };

    // 便捷函数
    inline Ref<Texture2D> get_key_texture(Key key) {
        return KeycodeTextureList::get_singleton().get_texture(key);
    }
}

#endif // KEYCODE_TEXTURES_HPP