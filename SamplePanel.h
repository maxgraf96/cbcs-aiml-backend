/*
  ==============================================================================

    SamplePanel.h
    Created: 15 Mar 2020 3:40:25pm
    Author:  Music

  ==============================================================================
*/

#pragma once
#ifndef DMLAP_BACKEND_SAMPLE_PANEL_H
#define DMLAP_BACKEND_SAMPLE_PANEL_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "Analyser.h"
#include "Traverser.h"

using namespace juce;
//==============================================================================
/*
*/
class SamplePanel : public Component, FilenameComponentListener, private Value::Listener
{
public:
    SamplePanel(Analyser&, Traverser&);
    ~SamplePanel() override;

    void paint (Graphics&) override;
    void resized() override;
	// Set the file path to current sample
    void setCurrentFilePath(String& path);

	// ID of the currentFilePath value
	// This is needed to load a file after the plugin's state is initialised
	// i.e. if this particular instance of the plugin was used e.g. in a DAW and is then reloaded
    static Identifier currentFilePathID;
    Value currentFilePath;
private:
    Analyser& analyser;
    Traverser& traverser;

    // State for file loaded
    enum FileLoadedState { notLoaded, loading, loaded, rejected, dbLoaded };
    FileLoadedState fileLoadedState;

    unique_ptr<FileChooser> dirChooser;
    void mouseDown (const MouseEvent& event) override;

    AudioFormatManager formatManager;

    // Component for loading files
    std::unique_ptr<FilenameComponent> filenameComponent;

    // Pointer to AudioBuffer in procesor that holds the samples once a file is loaded
    std::unique_ptr<AudioBuffer<float>> sampleBuffer;

	// Painting directive if no sample is loaded
    void paintIfNoFileLoaded(Graphics& g);
	// Painting directive if a sample is loaded
    void paintIfFileLoaded(Graphics& g);

	// Callback that's triggered if a new file is loaded
    void filenameComponentChanged(FilenameComponent* fileComponentThatHasChanged) override;
	// Load a directory or file to the sample buffer
    void loadFile(const File& file);
    // Helper function - load a single ".wav" file
    void loadSingleFile(const File& file);

	// Callback currently only to listen to file-path changes
    void valueChanged(Value& val) override;

	// Colours
	Colour backgroundColour = Colour(0xff220901);
	Colour timeMarkerColour = Colour(0xff5EB0A7);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplePanel)
};

#endif