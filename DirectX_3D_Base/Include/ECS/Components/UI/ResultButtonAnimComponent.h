#ifndef ___RESULTBUTTON_COMPONENT_H___
#define ___RESULTBUTTON_COMPONENT_H___

struct ResultButtonAnimComponent
{
    float animTimer = 0.0f;

    // ボタンの基準サイズ
    float baseW = 0.0f;
    float baseH = 0.0f;

    // 今カーソルが合っているか
    bool isSelected = false;
};


#endif //___RESULTBUTTON_COMPONENT_H___