#pragma once
#ifndef DMLAP_BACKEND_MAIN_H
#define DMLAP_BACKEND_MAIN_H

#include <chrono>
#include <thread>
#include <juce_gui_extra/juce_gui_extra.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <juce_osc/juce_osc.h>
#include "external_libraries/essentia/include/algorithmfactory.h"
#include "DataLoaderPanel.h"
#include "DBConnector.h"
#include "Analyser.h"
#include "Traverser.h"
#include "Constants.h"
#include "Utility.h"
#include "MyLookAndFeel.h"

using namespace std::chrono;
using namespace juce;
using namespace std;
using namespace essentia;
using namespace essentia::standard;

//==============================================================================
/*
    The main component of the JUCE application.
*/
class MainComponent  :
        public juce::AudioAppComponent,
        public juce::ChangeListener,
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
    /**
     * This is the main "initialise" method of the MainComponent. It is called every time the audio context changes
     * i.e. the audio device changes, switching to headphones, etc. When it is called it initialises all the sub-components
     * (Analyser, Traverser, DBConnector)
     * @param samplesPerBlockExpected Samples per audio block of the new context
     * @param sampleRate Audio sample rate of the new context
     */
    void prepareToPlay (int samplesPerBlockExpected, double sampleRate) override;

    /**
     * This is the main audio processing method. All audio recording and playback is handled here
     * @param bufferToFill The buffer to read audio data from (in case of recording) or to write audio data to (in case of playback)
     */
    void getNextAudioBlock (const juce::AudioSourceChannelInfo& bufferToFill) override;

    void releaseResources() override;

    //==============================================================================
    /**
     * Paints the GUI
     * @param g Graphics context
     */
    void paint (juce::Graphics& g) override;
    void resized() override;

    /**
     * Callback for button clicks
     * @param button
     */
    void buttonClicked (juce::Button* button) override;

    /**
     * Callback for key presses
     * @param key
     * @param originatingComponent
     * @return
     */
    bool keyPressed(const KeyPress &key, Component *originatingComponent) override;

    /**
     * Callback for handling asynchronous UI updates
     */
    void handleAsyncUpdate() override;

private:
    //==============================================================================
    // DB connection
    unique_ptr<DBConnector> dbConnector;
    // The data loader panel component
    unique_ptr<DataLoaderPanel> dataLoaderPanel;
    // The analyser (for calculating audio features)
    unique_ptr<Analyser> analyser;
    // The traverser (for creating trajectories through the grain sound space)
    unique_ptr<Traverser> traverser;

    // Audio data of the generated trajectory grains
    unique_ptr<AudioBuffer<float>> generatedBuffer;
    // Grain data of the generated trajectory grains
    vector<Grain> generatedGrains;
    // Current index of the generatedBuffer audio buffer
    atomic<int> generatedIdx;

    // Flag for checking if we're currently looping (i.e. playing with the little ball thingimajic) via the MOBILE OSC interface
    bool isLooping = false;
    int loopingGrainStartIdx = -1;
    int loopingGrainEndIdx = -1;

    // Flag for checking whether generated buffer is currently looping
    // This is mainly for listening to created loops
    bool isGeneratedLooping = false;

    // GUI
    // Custom look and feel of JUCE GUI
    MyLookAndFeel myLookAndFeel;
    // Colours
    Colour recordButtonBackground = Colour::fromString("FFFF605C");
    Colour playButtonBackground = Colour::fromString("FF00CA4E");
    Colour feedbackButtonBackground = Colour::fromString("FF2f3640");
    Colour black = Colour::fromRGB(0, 0, 0);

    // Buttons
    unique_ptr<TextButton> recordButton;
    unique_ptr<TextButton> playButton;
    unique_ptr<TextButton> yesButton;
    unique_ptr<TextButton> noButton;
    unique_ptr<TextButton> superlikeButton;
    unique_ptr<TextButton> superdislikeButton;
    unique_ptr<TextButton> runAgentButton;
    unique_ptr<TextButton> exploreButton;
    unique_ptr<TextButton> resetButton;

    /**
     * Initialise all GUI elements
     */
    void initialiseGUI();

    // OSC for communication with python RL agent
    OSCSender sender;
    int oscSenderPort = 5005;
    int oscListeningPort = 12000;
    void showConnectionErrorMessage (const String& messageText);
    void oscMessageReceived (const juce::OSCMessage& message) override;
    int oscCounter = 0;

    // RL management
    bool isAgentPaused = true;
    bool shouldPrimeTrajectory = false;
    /**
     * Prime RL agent with current trajectory
     */
    void primeTrajectory();

    // Recording audio
    unique_ptr<AudioBuffer<float>> recordingBuffer;
    atomic<int> recordingIdx;
    bool isRecording = false;

    // Audio device manager
    bool isManagerVisible = false;
    AudioDeviceSelectorComponent audioSetupComp;
    void setupDiagnosticsAndDeviceManager();
    void logMessage(const String& m);
    void dumpDeviceInfo();
    juce::TextEditor diagnosticsBox;
    void changeListenerCallback(ChangeBroadcaster*) override;

    // Thread safety
    atomic<bool> canSetGeneratedIdx;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

#endif

