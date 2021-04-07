//
// Created by Max on 07/04/2021.
//

#include "MyLookAndFeel.h"

void MyLookAndFeel::drawButtonBackground(Graphics& g, Button& button, const Colour& backgroundColour,
                                         bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) {
    auto buttonArea = button.getLocalBounds();
    Colour background = shouldDrawButtonAsDown ? backgroundColour.darker(0.4f) : backgroundColour;
    g.setColour (background);
    g.fillRect (buttonArea);

    // Draw border
    button.setColour(ComboBox::outlineColourId, Colour::fromRGB(0, 0, 0));
    g.setColour (button.findColour (ComboBox::outlineColourId));
    g.drawRect(buttonArea.toFloat(), 0.3f);
}

Font MyLookAndFeel::getTextButtonFont(TextButton &, int buttonHeight) {
    return {jmin(22.0f, static_cast<float>(buttonHeight) * 0.8f)}; // default
}