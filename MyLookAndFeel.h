//
// Created by Max on 07/04/2021.
//

#ifndef DMLAP_BACKEND_MYLOOKANDFEEL_H
#define DMLAP_BACKEND_MYLOOKANDFEEL_H
#include <juce_gui_extra/juce_gui_extra.h>
using namespace juce;

class MyLookAndFeel : public LookAndFeel_V4 {
    public:
        void drawButtonBackground(
                Graphics&,
                Button &,
                const Colour& backgroundColour,
                bool shouldDrawButtonAsHighlighted,
                bool shouldDrawButtonAsDown
        ) override;
        Font getTextButtonFont(TextButton &, int buttonHeight) override;
};


#endif //DMLAP_BACKEND_MYLOOKANDFEEL_H
