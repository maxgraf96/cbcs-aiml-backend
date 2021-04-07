/*
  ==============================================================================

    SamplePanel.cpp
    Created: 15 Mar 2020 3:40:25pm
    Author:  Music

  ==============================================================================
*/

#include "SamplePanel.h"

//==============================================================================
SamplePanel::SamplePanel(Analyser& analyser, Traverser& traverser) : analyser(analyser), traverser(traverser)
{
    // Initialise loaded state
    fileLoadedState = notLoaded;

    // Set default location
    File* defaultLocation = new File("/Users/max/Music/Ableton/Samples");

    // Preload sample for convenience
	// Note: If this path does not exist the plugin simply defaults to no sample
    // const auto currentFile = new File("C:\\Users\\Music\\Ableton\\Mixes\\sweetrelease.wav");
    // filenameComponent->setCurrentFile(*currentFile, false, dontSendNotification);
    // loadFile(*currentFile);

    // Prepare waveform display
    formatManager.registerBasicFormats();

    sampleBuffer = make_unique<AudioBuffer<float>>();

    if(traverser.isMinMaxInitialised()){
        fileLoadedState = dbLoaded;
    }

    dirChooser = make_unique<FileChooser>("Select directories", *defaultLocation, "*.wav", true);

    // Set component size
    setSize(1024, 100);
}

SamplePanel::~SamplePanel()
{
}

void SamplePanel::paint (Graphics& g)
{
    g.fillAll (backgroundColour);

    //g.setColour (Colours::grey);
    //g.drawRect (getLocalBounds(), 1);

    g.setColour (Colours::white);
    g.setFont(24.0f);

	// Text to display on sample panel depends on whether a file is loaded
    String fileLoadedText = "";
    if (fileLoadedState == notLoaded) {
        fileLoadedText = "No sample loaded";
    }
    else if (fileLoadedState == loading) {
        fileLoadedText = "Drop sample here";
    }
    else if (fileLoadedState == rejected) {
        fileLoadedText = "Only '.wav' files are supported!";
    } else if(fileLoadedState == dbLoaded){
        fileLoadedText = "Database loaded.";
    }
    else {
        fileLoadedText = "Loaded";
    }
    g.drawText (fileLoadedText, getLocalBounds(),
                Justification::centred, true);   

    if (fileLoadedState == notLoaded)
        paintIfNoFileLoaded(g);
    else
        paintIfFileLoaded(g);
}

void SamplePanel::paintIfNoFileLoaded(Graphics& g)
{
    g.setColour(backgroundColour);
    g.setColour(Colours::white);
	g.setFont(24.0f);
}

void SamplePanel::paintIfFileLoaded(Graphics& g)
{
    // Draw background
    g.setColour(backgroundColour);
}

void SamplePanel::resized()
{
}

void SamplePanel::loadFile(const File& file) {
	// Get file extension
	const auto fileExtension = file.getFileExtension();
    // Only accept wavs
    if(file.isDirectory()){
        Array<File> wavs = file.findChildFiles(File::TypesOfFileToFind::findFiles, true, "*.wav");
        int counter = 1;
        juce::Logger::outputDebugString("Loading directory " + file.getFullPathName() + " with " + to_string(wavs.size()) + " files.");
        for(File& wav : wavs){
            juce::Logger::outputDebugString("   Loading file " + to_string(counter++) + " of " + to_string(wavs.size()));
            loadSingleFile(wav);
        }

        // Set file loaded state
        fileLoadedState = loaded;

    }
    else if (fileExtension == ".wav" || fileExtension == ".WAV") {
        // Load single file
        loadSingleFile(file);

        // Set file loaded state
        fileLoadedState = loaded;
    }
    else {
        // Show info text
        fileLoadedState = rejected;
    }
    repaint();
}

void SamplePanel::loadSingleFile(const File& file){
    ScopedPointer<AudioFormatReader> reader = formatManager.createReaderFor(file);
    // If file is valid load it and send callback to processor
    if (reader != 0)
    {
        // Fix channels to 2 (stereo)
        // Read into sample buffer
        const auto numChannels = 2;
        sampleBuffer->setSize(numChannels, reader->lengthInSamples);
        reader->read(sampleBuffer.get(), 0, reader->lengthInSamples, 0, true, true);
        // If the incoming sound is mono, use the same data for both channels
        if (reader->numChannels == 1)
        {
            const auto leftChannelReader = sampleBuffer->getReadPointer(0);
            const auto rightChannelWriter = sampleBuffer->getWritePointer(1);
            for (int sample = 0; sample < sampleBuffer->getNumSamples(); sample++)
            {
                rightChannelWriter[sample] = leftChannelReader[sample];
            }
        }

        // Analyse with essentia
        analyser.analyseAndSaveToDB(*sampleBuffer, file.getFileName().toStdString(),
                                    file.getFullPathName().toStdString());
    }
}

void SamplePanel::mouseDown (const MouseEvent& event) {
    if (dirChooser->browseForMultipleFilesOrDirectories())
    {
        Array<File> files = dirChooser->getResults();
        juce::Logger::outputDebugString("Loading " + to_string(files.size()) + " files/directories...");
        int counter = 1;
        for(const auto& file : files){
            juce::Logger::outputDebugString(to_string(counter++) + " / " + to_string(files.size()));
            loadFile(file);
        }
        juce::Logger::outputDebugString("Done loading.");

        // Calculate feature statistics
        traverser.calculateFeatureStatistics();
    }
}