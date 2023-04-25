//Szymon Januszek 2023
#include <bits/stdc++.h>

using namespace std;

void panic(const string &msg) {
    cerr << msg << endl;
    exit(1);
}

inline float mod(float val, int m){
    while(val < 0) val += m;
    while(val > m) val -= m;
    return val;
}

const std::regex COLOR_DEC("^([0-9]{1,3},){3}([0-9]{1,3})$");
const std::regex COLOR_HEX("^([0-9a-f]{3}|[0-9a-f]{6}|[0-9a-f]{8})$");

enum Modes {
    MIX,
    LOWEST,
    HIGHEST,
    MIX_SAT
};

struct Color;

struct ColorHSL {
    float hue = 0, sat = 0, light = 0, alpha = 1;
    ColorHSL() = default;
    ColorHSL(const Color &color);
};

struct Color {
    uint8_t alpha = 0xFF, blue = 0, green = 0, red = 0;

    Color() = default;
    Color(uint32_t r, uint32_t g, uint32_t b, uint32_t a = 255): red(r), green(g), blue(b), alpha(a) {};
    Color(const ColorHSL &hsl);
} __attribute__((packed)); //allow for bit magick

ColorHSL::ColorHSL(const Color &color) {
    //Standard RGB -> HSL conversion procedure (consult wikipedia)
    alpha = color.alpha / 255.0f;

    float R = color.red / 255.0f, G = color.green / 255.0f, B = color.blue / 255.0f;
    float M = max({R, G, B}), m = min({R, G, B});
    float c = (float)M - m;

    if(c != 0)  {
        if(M == R) hue = 60.0f * mod((G - B) / c, 6);
        else if(M == G) hue = 120 + 60.0f * (B - R) / c;
        else if(M == B) hue = 240 + 60.0f * (R - G) / c;
    }

    light = (M + m) * 0.5f;

    if(light == 0 || light == 1) sat = 0;
    else sat = c / (1 - abs(2 * light - 1));
}

Color::Color(const ColorHSL &hsl) {
    //HSL -> RGB conversion
    alpha = (uint8_t)(hsl.alpha * 255);

    float c = (1 - abs(2*hsl.light)) * hsl.sat;
    float h = hsl.hue / 60.0f;
    float x = c * (1 - abs(mod(h, 2) - 1));

    float r,g,b;

    if(h < 2) {
        b = 0;
        r = x;
        g = c;
        if(h < 1) swap(r, g);
    } else if(h < 4) {
        r = 0;
        g = x;
        b = c;
        if(h < 3) swap(g, b);
    } else {
        g = 0;
        r = c;
        b = x;
        if(h < 5) swap(r, b);
    }

    float m = hsl.light - (c / 2);

    r += m;
    g += m;
    b += m;

    red = (uint8_t)round(r * 255);
    green = (uint8_t)round(g * 255);
    blue = (uint8_t)round(b * 255);
}

/**
 * This function parses color string into Color object
 * @param c - mutable reference to resulting color
 * @param word - C-string to convert
 * @return True - for success, False - if something goes wrong
 */
bool parse_color(Color &c, const string &word) {
    if(std::regex_match(word, COLOR_HEX)) {
        //Matches hex format specification, treat as alpha packed integer
        uint32_t val;

        if(sscanf(word.c_str(), "%x", &val) != 1) return false;

        switch (word.length()) {
            //12-bit RGB
            case 3: {
                c = Color(((val >> 8) & 0xF) * 16, ((val >> 4) & 0xF) * 16, (val & 0xF) * 16, 255);
                return true;
            }
            //24-bit RGB, convert to 32-bit RGBA
			[[fallthrough]] 		
            case 6: val = (val << 8) | 0xFF; 
            //32-bi RGBA. Convert to packed color
            case 8: {
                c = *reinterpret_cast<Color*>(&val);
                return true;
            }
            default: return false;
        }
    }

    if(std::regex_match(word, COLOR_DEC)) {
        //decimal with commas
        int R, G, B, A;
        if(sscanf(word.c_str(), "%d,%d,%d,%d", &R, &G, &B, &A) != 4) return false;
        if(R > 255 | G > 255 | B > 255 | A > 255) return false;
        c = Color(R, G, B, A);
        return true;
    }

    return false;
}

tuple<Color, ColorHSL> compute_mix(const vector<Color> &colors) {
    unsigned int R = 0, G = 0, B = 0, A = 0;

    for(auto i: colors) {
        R += i.red;
        G += i.green;
        B += i.blue;
        A += i.alpha;
    }

    Color avgRgb(
            (int)round(R / (double)colors.size()),
            (int)round(G / (double)colors.size()),
            (int)round(B / (double)colors.size()),
            (int)round(A / (double)colors.size())
    );

    return {avgRgb, avgRgb};
}

tuple<Color, ColorHSL> compute_mix_saturate(const vector<Color> &colors) {
    auto avgRgb = get<0>(compute_mix(colors));

    float avg_sat = 0;
    for(auto i: colors) avg_sat += ColorHSL(i).sat;

    ColorHSL result = avgRgb;
    result.sat = avg_sat / (float)colors.size();

    return {result, result};
}

tuple<Color, ColorHSL> compute_lowest(const vector<Color> &colors) {
    uint8_t r = 255, g = 255, b = 255, a = 255;

    for(auto i: colors) {
        r = min(r, i.red);
        g = min(g, i.green);
        b = min(b, i.blue);
        a = min(a, i.alpha);
    }

    return {Color(r,g,b,a), Color(r,g,b,a)};
}

tuple<Color, ColorHSL> compute_highest(const vector<Color> &colors) {
    uint8_t r = 0, g = 0, b = 0, a = 0;

    for(auto i: colors) {
        r = max(r, i.red);
        g = max(g, i.green);
        b = max(b, i.blue);
        a = max(a, i.alpha);
    }

    return {Color(r,g,b,a), Color(r,g,b,a)};
}

int main(int argn, char *argc[]) {
    vector<Color> colors;
    Modes mode = MIX;

    //parse input arguments
    bool lookaheadForMode = false;
    for(int i = 1; i < argn; i++) {
        const string param(argc[i]);

        if(lookaheadForMode) {
            if (param == "mix") mode = MIX;
            else if (param == "lowest") mode = LOWEST;
            else if (param == "highest") mode = HIGHEST;
            else if (param == "mix-saturate") mode = MIX_SAT;
            else cerr << "Unrecognized mode: \"" << param << "\"" << endl;
            lookaheadForMode = false;
        }else if(param == "-m" || param == "--mode") lookaheadForMode = true;
        else {
            Color c;
            if(parse_color(c, param)) colors.push_back(c);
            else panic("Unrecognized value: \"" + param + "\"");
        }
    }

    ifstream file;
    file.open("colors.txt");

    if(!file.is_open()) panic("Couldn't open the file!");

    while(file) {
        string line;
        getline(file, line);

        Color c;
        //ignore malformed lines
        if(parse_color(c, line)) colors.push_back(c);
    }

    file.close();

    if(colors.empty()) panic("No colors provided!");

    Color finalColor;
    ColorHSL finalColorHSL;

    if(mode == Modes::MIX) tie(finalColor, finalColorHSL) = compute_mix(colors);
    else if(mode == Modes::MIX_SAT) tie(finalColor, finalColorHSL) = compute_mix_saturate(colors);
    else if(mode == Modes::LOWEST) tie(finalColor, finalColorHSL) = compute_lowest(colors);
    else tie(finalColor, finalColorHSL) = compute_highest(colors); //Mode::HIGHEST

    printf("RED: %d\nGREEN: %d\nBLUE: %d\nALPHA: %d\nHEX: #%x\nHUE: %.2f\nSATURATION: %.2f\nLIGHTNESS: %.2f\n",
           finalColor.red,
           finalColor.green,
           finalColor.blue,
           finalColor.alpha,
           *(uint32_t*)(void*)&finalColor,
           finalColorHSL.hue,
           finalColorHSL.sat,
           finalColorHSL.light
           );

    return 0;
}
