#pragma once
#ifndef DMLAP_BACKEND_MAIN_H
#define DMLAP_BACKEND_MAIN_H

// CMake builds don't use an AppConfig.h, so it's safe to include juce module headers
// directly. If you need to remain compatible with Projucer-generated builds, and
// have called `juce_generate_juce_header(<thisTarget>)` in your CMakeLists.txt,
// you could `#include <JuceHeader.h>` here instead, to make all your module headers visible.
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_osc/juce_osc.h>
#include "external_libraries/essentia/include/algorithmfactory.h"
#include "SamplePanel.h"
#include "DBConnector.h"
#include "Analyser.h"
#include "Traverser.h"
#include "Constants.h"
#include "Utility.h"


using namespace juce;
using namespace std;
using namespace essentia;
using namespace essentia::standard;

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent  :
        public juce::AudioAppComponent,
        public juce::Button::Listener,
        public AsyncUpdater,
        private OSCReceiver,
        private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::MessageLoopCallback>,
        public KeyListener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;
    void releaseResources() override;

    //==============================================================================
    void paint (juce::Graphics& g) override;
    void resized() override;

    void buttonClicked (juce::Button* button) override;
    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;
    void handleAsyncUpdate() override;

private:
    //==============================================================================
    // Your private member variables go here...
    unique_ptr<DBConnector> dbConnector;
    unique_ptr<SamplePanel> samplePanel;
    unique_ptr<Analyser> analyser;
    unique_ptr<Traverser> traverser;

    AudioBuffer<float> generated;
    int generatedIdx = -1;

    // GUI
    unique_ptr<TextButton> playButton;
    unique_ptr<TextButton> yesButton;
    unique_ptr<TextButton> noButton;
    unique_ptr<TextButton> resetButton;
    void initialiseGUI();

    unique_ptr<Label> recordingLabel;

    // OSC for communication with python RL agent
    OSCSender sender;
    int oscSenderPort = 5005;
    int oscListeningPort = 12000;
    void showConnectionErrorMessage (const String& messageText);
    void oscMessageReceived (const juce::OSCMessage& message) override;

    // RL management
    String modelPath = "";

    // Recording audio
    AudioBuffer<float> recordingBuffer;
    int recordingIdx = -1;
    bool isRecording = false;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

#endif

