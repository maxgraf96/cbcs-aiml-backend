#pragma once
#ifndef DMLAP_BACKEND_SAMPLE_PANEL_H
#define DMLAP_BACKEND_SAMPLE_PANEL_H

#include <juce_audio_utils/juce_audio_utils.h>
#include "Analyser.h"
#include "Traverser.h"

using namespace juce;

/**
 * Component for loading ".wav" files into the database. Contains both backend and GUI functionality.
 */
class DataLoaderPanel : public Component
{
public:
    DataLoaderPanel(Analyser&, Traverser&);
    ~DataLoaderPanel() override;

    void paint (Graphics&) override;
    void resized() override;

private:
    // Ref to analyser
    Analyser& analyser;
    // Ref to traverser
    Traverser& traverser;

    // State for file loaded
    enum FileLoadedState { notLoaded, loading, loaded, rejected, dbLoaded };
    FileLoadedState fileLoadedState;

    // JUCE component to choose files
    unique_ptr<FileChooser> dirChooser;

    /**
     * Mouse down callback
     * @param event
     */
    void mouseDown (const MouseEvent& event) override;

    // Format manager to deal with ".wav" files
    AudioFormatManager formatManager;

    // Pointer to AudioBuffer in MainComponent that holds the samples once a file is loaded
    std::unique_ptr<AudioBuffer<float>> sampleBuffer;

	// Painting directive if db is not loaded
    void paintIfNoFileLoaded(Graphics& g);
	// Painting directive if db is loaded
    void paintIfFileLoaded(Graphics& g);

	// Load a directory or file to the db
    void loadFile(const File& file);
    // Helper function - load a single ".wav" file
    void loadSingleFile(const File& file);

	// Colours
	Colour backgroundColour = Colour(0xff220901);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (DataLoaderPanel)
};

#endif